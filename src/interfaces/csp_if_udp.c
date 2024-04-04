#include <csp/interfaces/csp_if_udp.h>

#include <csp/csp_debug.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#include <csp/csp.h>
#include <endian.h>
#include <csp/csp_interface.h>
#include <csp/csp_id.h>

#ifndef MSG_CONFIRM
#define MSG_CONFIRM (0)
#endif

static int csp_if_udp_tx(csp_iface_t * iface, uint16_t via, csp_packet_t * packet, int from_me) {

	csp_if_udp_conf_t * ifconf = iface->driver_data;

	if (ifconf->sockfd == 0) {
		csp_print("Sockfd null\n");
		csp_buffer_free(packet);
		return CSP_ERR_NONE;
	}

	csp_id_prepend(packet);
	ifconf->peer_addr.sin_family = AF_INET;
	ifconf->peer_addr.sin_port = htons(ifconf->rport);
	sendto(ifconf->sockfd, packet->frame_begin, packet->frame_length, MSG_CONFIRM, (struct sockaddr *)&ifconf->peer_addr, sizeof(ifconf->peer_addr));
	csp_buffer_free(packet);

	return CSP_ERR_NONE;
}

int csp_if_udp_rx_work(int sockfd, size_t unused, csp_iface_t * iface) {

	csp_packet_t * packet = csp_buffer_get(0);
	if (packet == NULL) {
		return CSP_ERR_NOMEM;
	}

	/* Setup RX frane to point to ID */
	int header_size = csp_id_setup_rx(packet);
	int received_len = recvfrom(sockfd, (char *)packet->frame_begin, sizeof(packet->data) + header_size, MSG_WAITALL, NULL, NULL);
	
	if (received_len <= 4) {
		csp_buffer_free(packet);
		return CSP_ERR_NOMEM;
	}

	packet->frame_length = received_len;

	/* Parse the frame and strip the ID field */
	if (csp_id_strip(packet) != 0) {
		csp_buffer_free(packet);
		return CSP_ERR_INVAL;
	}

	csp_qfifo_write(packet, iface, NULL);

	return CSP_ERR_NONE;
}

void * csp_if_udp_rx_loop(void * param) {

	csp_iface_t * iface = param;
	csp_if_udp_conf_t * ifconf = iface->driver_data;

	while (ifconf->sockfd == 0) {

		ifconf->sockfd = socket(AF_INET, SOCK_DGRAM, PF_PACKET);

		struct sockaddr_in server_addr = {0};
		server_addr.sin_family = AF_INET;
		server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		server_addr.sin_port = htons(ifconf->lport);

		bind(ifconf->sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));

		if (ifconf->sockfd < 0) {
			csp_print("  UDP server waiting for port %d\n", ifconf->lport);
			sleep(1);
			continue;
		}
		break;
	}

	while (1) {
		int ret;
		ret = csp_if_udp_rx_work(ifconf->sockfd, 0, iface);
		if (ret == CSP_ERR_INVAL) {
			iface->rx_error++;
		} else if (ret == CSP_ERR_NOMEM) {
			usleep(10000);
		}
	}

	return NULL;
}

void csp_if_udp_init(csp_iface_t * iface, csp_if_udp_conf_t * ifconf) {

	pthread_attr_t attributes;
	int ret;

	iface->driver_data = ifconf;

	if (inet_aton(ifconf->host, &ifconf->peer_addr.sin_addr) == 0) {
		csp_print("  Unknown peer address %s\n", ifconf->host);
	}

	csp_print("  UDP peer address: %s:%d (listening on port %d)\n", inet_ntoa(ifconf->peer_addr.sin_addr), ifconf->rport, ifconf->lport);

	/* Start server thread */
	ret = pthread_attr_init(&attributes);
	if (ret != 0) {
		csp_print("csp_if_udp_init: pthread_attr_init failed: %s: %d\n", strerror(ret), ret);
	}
	ret = pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);
	if (ret != 0) {
		csp_print("csp_if_udp_init: pthread_attr_setdetachstate failed: %s: %d\n", strerror(ret), ret);
	}
	ret = pthread_create(&ifconf->server_handle, &attributes, csp_if_udp_rx_loop, iface);
	if (ret != 0) {
		csp_print("csp_if_udp_init: pthread_create failed: %s: %d\n", strerror(ret), ret);
	}
	ret = pthread_attr_destroy(&attributes);
	if (ret != 0) {
		csp_print("csp_if_udp_init: pthread_attr_destroy failed: %s: %d\n", strerror(ret), ret);
	}

	/* Regsiter interface */
	iface->name = "UDP",
	iface->nexthop = csp_if_udp_tx,
	csp_iflist_add(iface);
}
