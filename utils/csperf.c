/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2016 GomSpace ApS (http://www.gomspace.com)
Copyright (C) 2016 AAUSAT3 Project (http://aausat3.space.aau.dk)

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

/* CSP performance testing tool. Heavily inspired by iperf and friends.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

#include <csp/csp.h>
#include <csp/interfaces/csp_if_can.h>

#define CSPERF_TYPE_BEGIN	0x01
#define CSPERF_TYPE_DATA	0x02
#define CSPERF_TYPE_END		0x03

struct csperf_packet {
	uint8_t type;
} __attribute__ ((packed));

void usage(const char *argv0)
{
	printf("usage: %s [-s|-c host]\n", argv0);
}

void help(const char *argv0)
{
	usage(argv0);
	printf("  -a, --address ADDRESS    Set address\r\n");
	printf("  -s, --server             Run in server mode\n");
	printf("  -h, --help               Print help and exit\r\n");
}

int run_server(uint8_t port)
{
	csp_socket_t *socket;
	csp_conn_t *conn;

	printf("Server listening on port %hhu\n", port);

	return 0;
}

int run_client(uint8_t server, uint8_t port)
{
	printf("Client connecting to %hhu port %hhu\n", server, port);

	return 0;
}

int main(int argc, char **argv)
{
	int c;
	int option_index = 0;
	struct csp_can_config conf = {.ifc = "vcan0"};

	uint8_t addr = 8;
	uint8_t port = 21;
	uint8_t server = 9;
	int server_mode = 0;
	int client_mode = 0;

	struct option long_options[] = {
		{"address", required_argument, NULL,        'a'},
		{"client",  required_argument, NULL,        'a'},
		{"port",    required_argument, NULL,        'a'},
		{"help",    no_argument,       NULL,        'h'},
		{"server",  no_argument,       &server_mode, 1},
	};

	while ((c = getopt_long(argc, argv, "a:c:p:sh",
				long_options, &option_index)) != -1) {
		switch (c) {
		case 'a':
			addr = atoi(optarg);
			break;
		case 'c':
			client_mode = 1;
			server = atoi(optarg);
			break;
		case 'p':
			port = atoi(optarg);
			break;
		case 's':
			server_mode = 1;
			break;
		case 'h':
			help(argv[0]);
			exit(EXIT_SUCCESS);
		case '?':
		default:
			exit(EXIT_FAILURE);
		}
	}

	/* Server OR client must be selected */
	if (!(server_mode ^ client_mode)) {
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	csp_buffer_init(100, 512);
	csp_init(addr);

	csp_can_init(CSP_CAN_MASKED, &conf);
	csp_route_set(CSP_DEFAULT_ROUTE, &csp_if_can, CSP_NODE_MAC);

	csp_route_start_task(0, 0);

	if (server_mode) {
		run_server(port);
	} else {
		run_client(server, port);
	}

	return 0;
}
