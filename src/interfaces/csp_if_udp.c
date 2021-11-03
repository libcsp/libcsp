

#include <csp/interfaces/csp_if_udp.h>

#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <csp/csp.h>
#include <endian.h>
#include <csp/csp_interface.h>
#include <csp/arch/csp_thread.h>
#include <csp/csp_id.h>

#ifndef MSG_CONFIRM
#define MSG_CONFIRM (0)
#endif

static int csp_if_udp_tx(const csp_route_t * ifroute, csp_packet_t * packet) {

	csp_if_udp_conf_t * ifconf = ifroute->iface->driver_data;

	int sockfd;
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket creation failed");
		return CSP_ERR_BUSY;
	}

	csp_id_prepend(packet);

	ifconf->peer_addr.sin_family = AF_INET;
	ifconf->peer_addr.sin_port = htons(ifconf->rport);
	sendto(sockfd, packet->frame_begin, packet->frame_length, MSG_CONFIRM, (struct sockaddr *)&ifconf->peer_addr, sizeof(ifconf->peer_addr));
	csp_buffer_free(packet);

	close(sockfd);

	return CSP_ERR_NONE;
}

int csp_if_udp_rx_get_socket(int lport) {

	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	struct sockaddr_in server_addr = {0};
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(lport);

	return bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
}

int csp_if_udp_rx_work(int sockfd, size_t mtu, struct sockaddr_in * peer_addr, csp_iface_t * iface) {

	csp_packet_t * packet = csp_buffer_get(mtu);
	if (packet == NULL) {
		return CSP_ERR_NOMEM;
	}

	/* Setup RX frane to point to ID */
	int header_size = csp_id_setup_rx(packet);
	int received_len = recvfrom(sockfd, (char *)packet->frame_begin, mtu + header_size, MSG_WAITALL, (struct sockaddr *)peer_addr, NULL);
	packet->frame_length = received_len;

	csp_log_info("UDP peer address: %s", inet_ntoa(peer_addr->sin_addr));

	/* Parse the frame and strip the ID field */
	if (csp_id_strip(packet) != 0) {
		csp_buffer_free(packet);
		return CSP_ERR_INVAL;
	}

	csp_qfifo_write(packet, iface, NULL);

	return CSP_ERR_NONE;
}

CSP_DEFINE_TASK(csp_if_udp_rx_loop) {

	csp_iface_t * iface = param;
	csp_if_udp_conf_t * ifconf = iface->driver_data;
	int sockfd;

	do {
		sockfd = csp_if_udp_rx_get_socket(ifconf->lport);
		if (sockfd < 0) {
			printf("UDP server waiting for port %d\n", ifconf->lport);
			sleep(1);
		}
	} while (sockfd < 0);

	while (1) {
		int ret;

		ret = csp_if_udp_rx_work(sockfd, iface->mtu, &ifconf->peer_addr, iface);
		if (ret == CSP_ERR_INVAL) {
			iface->rx_error++;
		} else if (ret == CSP_ERR_NOMEM) {
			csp_sleep_ms(10);
		}
	}

	return CSP_TASK_RETURN;
}

void csp_if_udp_init(csp_iface_t * iface, csp_if_udp_conf_t * ifconf) {

	iface->driver_data = ifconf;

	if (inet_aton(ifconf->host, &ifconf->peer_addr.sin_addr) == 0) {
		printf("Unknown peer address %s\n", ifconf->host);
	}

	printf("UDP peer address: %s:%d (listening on port %d)\n", inet_ntoa(ifconf->peer_addr.sin_addr), ifconf->rport, ifconf->lport);

	/* Start server thread */
	int ret = csp_thread_create(csp_if_udp_rx_loop, "UDPS", 10000, iface, 0, &ifconf->server_handle);
	csp_log_info("csp_if_udp_rx_loop start %d\r\n", ret);

	/* MTU is datasize */
	iface->mtu = csp_buffer_data_size();

	/* Regsiter interface */
	iface->name = "UDP",
	iface->nexthop = csp_if_udp_tx,
	csp_iflist_add(iface);
}
