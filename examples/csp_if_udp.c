/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2012 GomSpace ApS (http://www.gomspace.com)
Copyright (C) 2012 AAUSAT3 Project (http://aausat3.space.aau.dk)

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
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <csp/csp.h>
#include <csp/csp_endian.h>
#include <csp/csp_interface.h>
#include <csp/arch/csp_thread.h>

#define MY_ADDRESS	1
csp_iface_t csp_if_udp;

int csp_if_udp_tx(csp_iface_t *ifc, csp_packet_t * packet, uint32_t timeout) {
	int sockfd;
   	int broadcast = 1;
	struct sockaddr_in peer_addr = {0};

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0 ) {
		csp_log_error("[Client] Socket creation failed");
		return CSP_ERR_BUSY;
	}

	peer_addr.sin_family = AF_INET;
	peer_addr.sin_port = htons(9600);
	peer_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
	if(setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast,sizeof(broadcast)) < 0) {
		csp_log_error("[Client] Error in setting Broadcast option");
		close(sockfd);
		return 0;
        }

	sendto(sockfd, (void *) &packet->id, packet->length, MSG_CONFIRM, (struct sockaddr *) &peer_addr, sizeof(peer_addr));
	close(sockfd);

	return CSP_ERR_NONE;
}

CSP_DEFINE_TASK(csp_if_udp_rx_task) {
	int sockfd;
	csp_iface_t * iface = param;
	struct sockaddr_in server_addr = {0};
   	int broadcast = 1;

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0 ) {
		csp_log_error("[Server] socket creation failed");
		return CSP_ERR_BUSY;
	}
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(9600);

    	if(setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0)
    	{

        	csp_log_error("[Server] Error in setting Broadcast option");
	        close(sockfd);
	        return 0;

    	}

	csp_log_info("[Server] UDP server starting");
	while(1) {

		csp_log_info("[Server] Binding socket");
		if (bind(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
			csp_log_warn("UDP server waiting for port 9600");
			sleep(1);
			continue;
		}

		csp_log_info("[Server] Socket Binded");
		while(1) {

			char buffer[300];
			csp_log_info("[Server] Waiting packet");
			// TODO set bufsize
			int received_len = recv(sockfd, (void *)buffer, 100, MSG_WAITALL);
			csp_log_info("[Server] Packet received %d bytes", received_len);

			/* Check for short */
			if (received_len < 4) {
				csp_log_error("[Server] Too short UDP packet");
				continue;
			}

			csp_packet_t * packet = csp_buffer_get(100);

			if (packet == NULL)
				continue;

			memcpy(&packet->id, buffer, received_len);
			csp_log_info("[Server] packet->data: %s", packet->data);
			csp_log_info("[Server] packet->id.src: %d", packet->id.src);
			csp_log_info("[Server] packet->id.sport: %d", packet->id.sport);
			packet->length = received_len;
			csp_buffer_free(packet);
		}

	}

	return CSP_TASK_RETURN;
}


CSP_DEFINE_TASK(task_client) {

	csp_packet_t * packet;

	while (1) {

		csp_sleep_ms(1000);

		/* Get packet buffer for data */
		packet = csp_buffer_get(100);
		if (packet == NULL) {
			/* Could not get buffer element */
			csp_log_error("[Client] Failed to get buffer element");
			return CSP_TASK_RETURN;
		}

		/* Fill csp header */
		packet->id.pri = CSP_PRIO_LOW;
		packet->id.src = 1;
		packet->id.dst = 2;
		packet->id.dport = CSP_ANY;
		packet->id.sport = CSP_ANY;
		packet->id.flags = 0;
		const char *msg = "Hello World\r";
		packet->length = CSP_HEADER_LENGTH + strlen(msg);

		memcpy((char *) packet->data, msg, strlen(msg));

		csp_log_info("[Client] packet length: %d", packet->length);
		csp_log_info("[Client] src: %d", packet->id.src);
		csp_log_info("[Client] src port: %d", packet->id.sport);
		csp_log_info("[Client] dst port: %d", packet->id.dport);
		csp_log_info("[Client] msg sended: %s", packet->data);
		csp_if_udp_tx(&csp_if_udp, packet, 1000);
		csp_log_info("[Client] Packet sended");
		csp_buffer_free(packet);

	}

	return CSP_TASK_RETURN;
}



int csp_udp_init(const char *hostname, unsigned int address)
{
	int ret;

	/* Initialize CSP */
	csp_set_address(address);
	csp_set_hostname(hostname);

	ret = csp_port_init();
	if (ret != CSP_ERR_NONE)
		return ret;

	ret = csp_qfifo_init();
	if (ret != CSP_ERR_NONE)
		return ret;

	/* MTU is datasize */
	csp_if_udp.mtu = csp_buffer_datasize();
	/* Register interface */
	csp_if_udp.name = "UDP";
	csp_if_udp.nexthop = csp_if_udp_tx;

	csp_iflist_add(&csp_if_udp);

	/* Register udp route */
	csp_route_set(csp_get_address(), &csp_if_udp, CSP_NODE_MAC);

	csp_route_set(CSP_DEFAULT_ROUTE, &csp_if_udp, CSP_NODE_MAC);

	/* Init buffer system with 10 packets of maximum 300 bytes each */
	csp_buffer_init(10, 300);

	return CSP_ERR_NONE;

}


int main(int argc, char **argv) {

	char * host = "127.0.0.1";
	static csp_thread_handle_t handle_server;

	csp_debug_toggle_level(2);
	csp_log_info("[Master] Initialising CSP");
	csp_udp_init(host, 1);

	/* Server */
	int ret = csp_thread_create(csp_if_udp_rx_task, "UDPS", 10000, &csp_if_udp, 0, &handle_server);
	csp_log_info("[Master] Server task started %d", ret);

	/* Client */
	ret = csp_thread_create(task_client, "UDPC", 10000, &csp_if_udp, 0, &handle_server);
	csp_log_info("[Master] Client task started");
	csp_route_start_task(500, 1);

	/* Wait for execution to end (ctrl+c) */
	while(1) {
		csp_conn_print_table();
		csp_route_print_table();
		csp_route_print_interfaces();
        	csp_sleep_ms(10000);
    	}
    	return 0;
}
