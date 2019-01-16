/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <csp/csp.h>
#include <csp/csp_interface.h>
#include <csp/arch/csp_thread.h>

struct sockaddr_in peer_addr = {0};

static int csp_if_udp_tx(csp_iface_t * interface, csp_packet_t * packet, uint32_t timeout) {
	int sockfd;
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
		perror("socket creation failed");
		return CSP_ERR_BUSY;
	}

	sendto(sockfd, (void *) &packet->id, packet->length + 4, MSG_CONFIRM, (struct sockaddr *) &peer_addr, sizeof(peer_addr));
	csp_buffer_free(packet);

	close(sockfd);

	return CSP_ERR_NONE;
}

CSP_DEFINE_TASK(csp_if_udp_rx_task) {

	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	struct sockaddr_in server_addr = {0};
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(9600);

	csp_iface_t * iface = param;

	while(1) {

		if (bind(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
			printf("UDP server waiting for port 9600\n");
			sleep(1);
			continue;
		}

		while(1) {

			char buffer[256];
			struct sockaddr_in client_addr = {0};
			unsigned int client_addr_len;
			int received_len = recvfrom(sockfd, (char *)buffer, 256, MSG_WAITALL, (struct sockaddr *) &client_addr, &client_addr_len);

			csp_packet_t * packet = csp_buffer_get(256);
			if (packet == NULL)
				continue;

			memcpy(&packet->id, buffer, received_len);
			packet->length = received_len - 4;

			csp_new_packet(packet, iface, NULL);


		}

	}

	return CSP_TASK_RETURN;

}

void csp_if_udp_init(csp_iface_t * iface, char * host) {

	peer_addr.sin_family = AF_INET;
	peer_addr.sin_port = htons(9610);

	if (inet_aton(host, &peer_addr.sin_addr) == 0) {
		printf("Invalid address %s\n", host);
	    return;
	}

	printf("UDP peer address: %s\n", inet_ntoa(peer_addr.sin_addr));

	/* Start server thread */
	static csp_thread_handle_t handle_server;
	int ret = csp_thread_create(csp_if_udp_rx_task, "UDPS", 10000, iface, 0, &handle_server);
	csp_log_info("csp_if_udp_rx_task start %d\r\n", ret);

	/* Regsiter interface */
	iface->name = "UDP",
	iface->nexthop = csp_if_udp_tx,
	csp_iflist_add(iface);

}
