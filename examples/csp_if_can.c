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

/* This example was tested using two usb2can connected in loopback, in posix.*/

#include <unistd.h>

#include <csp/csp.h>
#include <csp/csp_interface.h>
#include <csp/arch/csp_clock.h>
#include <csp/interfaces/csp_if_can.h>

#define TYPE_SERVER 1
#define TYPE_CLIENT 2
#define PORT        10
#define BUF_SIZE    250

char *message = "Testing CSP";
int me, other, type;
static csp_iface_t csp_interface;

void client_server_pseudo_task(void)
{
	csp_socket_t *sock;
	csp_packet_t *packet;


	if (type == TYPE_SERVER) {
		sock = csp_socket(CSP_SO_CONN_LESS);
		csp_bind(sock, PORT);
	}


	for (;;) {
		if (type == TYPE_CLIENT) {
			packet = csp_buffer_get(strlen(message));
			if (packet) {
				printf("Sending: %s, with %s\r\n", message, csp_interface.name);
				strcpy((char *) packet->data, message);
				packet->length = strlen(message);
				csp_sendto(CSP_PRIO_NORM, other, PORT, PORT, CSP_SO_NONE, packet, 1000);
			}
			sleep(1);
		} else {
			packet = csp_recvfrom(sock, 1000);
			if (packet) {
				printf("Received: %s, with %s\r\n", packet->data, csp_interface.name);
				csp_buffer_free(packet);
			}
		}
	}
}

int main(int argc, char **argv)
{
	struct csp_can_config can_conf0 = {.ifc = "can0"};
	struct csp_can_config can_conf1 = {.ifc = "can1"};

	if (argc != 2) {
		printf("usage: %s <server/client>\r\n", argv[0]);
		return -1;
	}

	/* Set type */
	if (strcmp(argv[1], "server") == 0) {
		me = 1;
		other = 2;
		type = TYPE_SERVER;
	} else if (strcmp(argv[1], "client") == 0) {
		me = 2;
		other = 1;
		type = TYPE_CLIENT;
	} else {
		printf("Invalid type. Must be either 'server' or 'client'\r\n");
		return -1;
	}

	/* Init CSP and CSP buffer system */
	if (csp_init(me) != CSP_ERR_NONE || csp_buffer_init(10, 300) != CSP_ERR_NONE) {
		return;
	}

	if (type == TYPE_CLIENT) {
		csp_can_init_ifc(&csp_interface, CSP_CAN_MASKED, &can_conf0);
	} else {
		csp_can_init_ifc(&csp_interface, CSP_CAN_MASKED, &can_conf1);
	}

	csp_route_set(other, &csp_interface, CSP_NODE_MAC);
	csp_route_start_task(0, 0);

	client_server_pseudo_task();
	return 0;
}
