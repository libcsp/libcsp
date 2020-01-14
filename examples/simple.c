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

#include <csp/csp.h>
#include <csp/arch/csp_thread.h>

/* CSP address - since this example runs both server and client in same process, the address is
   the same for both server and client */
#define MY_ADDRESS	1

/* Server port, the port the server listens on for incoming connections from the client. */
#define MY_PORT		10

/* Server task - handles requests from clients */
CSP_DEFINE_TASK(task_server) {

	csp_log_info("Server task started");

	/* Create socket with no specific socket options, e.g. accepts CRC32, HMAC, XTEA, etc. if enabled during compilation */
	csp_socket_t *sock = csp_socket(CSP_SO_NONE);

	/* Bind socket to all ports, e.g. all incoming connections will be handled here */
	csp_bind(sock, CSP_ANY);

	/* Create a backlog of 10 connections, i.e. up to 10 new connections can be queued */
	csp_listen(sock, 10);

	/* Wait for connections and then process packets on the connection */
	while (1) {

		/* Wait for a new connection, 10000 mS timeout */
		csp_conn_t *conn;
		if ((conn = csp_accept(sock, 10000)) == NULL) {
			/* timeout */
			continue;
		}

		/* Read packets on connection, timout is 100 mS */
		csp_packet_t *packet;
		while ((packet = csp_read(conn, 100)) != NULL) {
			switch (csp_conn_dport(conn)) {
			case MY_PORT:
				/* Process packet here */
				csp_log_protocol("Packet received on MY_PORT: %s", (char *) packet->data);
				csp_buffer_free(packet);
				break;

			default:
				/* Call the default CSP service handler, handle pings, buffer use, etc. */
				csp_service_handler(conn, packet);
				break;
			}
		}

		/* Close current connection */
		csp_close(conn);

	}

	return CSP_TASK_RETURN;

}
/* End of server task */

/* Client task sending requests to server task */
CSP_DEFINE_TASK(task_client) {

	csp_log_info("Client task started");

	while (1) {

		csp_sleep_ms(1000);

		/* Send ping to server, timeout 100 mS, ping size 100 bytes */
		int result = csp_ping(MY_ADDRESS, 100, 100, CSP_O_NONE);
		csp_log_protocol("Ping result %d [mS]", result);

		csp_sleep_ms(1000);

		/* Send data packet to server */

		/* Connect to host on MY_ADDRESS, port MY_PORT with regular UDP-like protocol and 1000 ms timeout */
		csp_conn_t * conn = csp_connect(CSP_PRIO_NORM, MY_ADDRESS, MY_PORT, 1000, CSP_O_NONE);
		if (conn == NULL) {
			/* Connect failed */
			csp_log_error("Connection failed");
			return CSP_TASK_RETURN;
		}

		/* Get packet buffer for message/data */
		csp_packet_t * packet = csp_buffer_get(100);
		if (packet == NULL) {
			/* Could not get buffer element */
			csp_log_error("Failed to get buffer element");
			return CSP_TASK_RETURN;
		}

		/* Copy data to packet */
		strcpy((char *) packet->data, "Hello World"); // strcpy adds a 0 termination

		/* Set packet length */
		packet->length = (strlen((char *) packet->data) + 1); // include the 0 termination

		/* Send packet */
		if (!csp_send(conn, packet, 1000)) {
			/* Send failed */
			csp_log_error("Send failed");
			csp_buffer_free(packet);
		}

		/* Close connection */
		csp_close(conn);

	}

	return CSP_TASK_RETURN;
}
/* End of client task */

/* main - initialization of CSP and start of server/client tasks */
int main(int argc, char * argv[]) {

	/**
	 * Initialise CSP,
	 * No physical interfaces are initialised in this example, e.g. only the loopback interface is used.
	 */

	/* Set default logging */
	csp_debug_set_level(CSP_INFO, true);
	csp_log_info("Initialising CSP");

	/* Init buffer system with 10 packets of maximum 300 bytes each */
	csp_buffer_init(5, 300);

	/* Init CSP with address MY_ADDRESS */
	csp_conf_t csp_conf;
	csp_conf_get_defaults(&csp_conf);
	csp_conf.address = MY_ADDRESS;
	csp_init(&csp_conf);

	/* Start router task with 500 word stack, OS task priority 1 */
	csp_route_start_task(500, 1);

	/* Enable debug output from CSP */
	if ((argc > 1) && (strcmp(argv[1], "-v") == 0)) {
		printf("Set logging\r\n");
                csp_debug_set_level(CSP_BUFFER, true);
                csp_debug_set_level(CSP_PACKET, true);
                csp_debug_set_level(CSP_PROTOCOL, true);
                csp_debug_set_level(CSP_LOCK, true);

		printf("Conn table\r\n");
		csp_conn_print_table();

		printf("Route table\r\n");
		csp_route_print_table();

		printf("Interfaces\r\n");
		csp_route_print_interfaces();
	}

	/* Start server thread */
	csp_thread_handle_t handle_server;
	csp_thread_create(task_server, "SERVER", 1000, NULL, 0, &handle_server);

	/* Start client thread */
	csp_thread_handle_t handle_client;
	csp_thread_create(task_client, "CLIENT", 1000, NULL, 0, &handle_client);

	/* Wait for execution to end (ctrl+c) */
	while(1) {
		csp_sleep_ms(100000);
	}

	return 0;

}
