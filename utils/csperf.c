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
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <assert.h>
#include <signal.h>
#include <getopt.h>

#include <csp/csp.h>
#include <csp/csp_endian.h>
#include <csp/interfaces/csp_if_can.h>

unsigned int timeout_ms = 1000;
unsigned int data_size = 100;
bool should_stop = false;
unsigned int runtime = 10;
unsigned int max_frames = 0;
unsigned int bandwidth = 1000000; /* bits/s */

#define CSPERF_TYPE_DATA 1
#define CSPERF_TYPE_DONE 2

struct csperf_packet {
	uint32_t seq;
	uint8_t type;
	uint8_t data[0];
} __attribute__ ((packed));

uint64_t get_nanosecs(void)
{
	struct timespec now;

	clock_gettime(CLOCK_MONOTONIC, &now);

	return now.tv_sec * 1000000000UL + now.tv_nsec;
}

void fill_pattern(uint8_t *dst, size_t len)
{
	int i;
	uint8_t b = 0x01;

	for (i = 0; i < len; i++)
		*dst++ = b++;
}

bool check_pattern(uint8_t *dst, size_t len)
{
	int i;
	uint8_t b = 0x01;

	for (i = 0; i < len; i++) {
		if (*dst++ != b++)
			return false;
	}

	return true;
}

void handle_connection(csp_conn_t *conn)
{
	csp_packet_t *packet;
	struct csperf_packet *pp;

	uint32_t seq;
	uint32_t last_seq = 0;
	bool valid;

	uint32_t received = 0;
	uint32_t lost = 0;
	uint32_t corrupt = 0;

	uint32_t last_update_seq = 0;
	uint64_t last_update_time = 0;

	printf("Connection from %hhu:%hhu\n",
	       csp_conn_src(conn), csp_conn_sport(conn));

	while (1) {
		packet = csp_read(conn, timeout_ms);
		if (!packet) {
			printf("Timeout\n");
			break;
		}

		if (packet->length != sizeof(*pp) + data_size)
			break;

		pp = (struct csperf_packet *) packet->data;
		seq = csp_ntoh32(pp->seq);
		valid = check_pattern(pp->data, data_size);

		received++;

		if (last_seq > 0 && seq != last_seq + 1)
			lost += seq - last_seq - 1;

		if (!valid) {
			printf("corrupted packet received!\n");
			corrupt++;
		}

		csp_buffer_free(packet);
	}

	printf("Received %u packets\n", received);
}

int run_server(uint8_t port)
{
	csp_socket_t *socket;
	csp_conn_t *conn;

	socket = csp_socket(CSP_SO_NONE);
	assert(socket);
	csp_bind(socket, port);
	csp_listen(socket, 10);

	printf("Server listening on port %hhu\n", port);

	while (1) {
		if ((conn = csp_accept(socket, 250)) == NULL) {
			if (should_stop)
				break;
			continue;
		}

		handle_connection(conn);

		csp_close(conn);
	}

	return 0;
}

int run_client(uint8_t server, uint8_t port)
{
	csp_conn_t *conn;
	csp_packet_t *packet;
	struct csperf_packet *pp;

	uint32_t seq = 0;
	uint64_t start_time, now;
	unsigned int sleep_time;

	printf("Client connecting to %hhu port %hhu\n", server, port);

	conn = csp_connect(CSP_PRIO_NORM, server, port, timeout_ms, CSP_O_NONE);
	if (!conn) {
		printf("Failed to connect to server\n");
		return -ENOTCONN;
	}

	/* Calculate time between packets in us for target speed */
	sleep_time = (sizeof(*pp) + data_size) * 8 * 1000000 / bandwidth;
	printf("Sleeping %u us between packets\n", sleep_time);

	start_time = get_nanosecs();

	while (1) {
		if (should_stop)
			break;

		now = get_nanosecs();
		if (now >= start_time + runtime * 1000000000UL) {
			printf("done\n");
			break;
		}

		packet = csp_buffer_get(sizeof(*pp) + data_size);
		if (!packet) {
			printf("Failed to allocate packet\n");
			return -ENOMEM;
		}

		pp = (struct csperf_packet *) packet->data;

		pp->seq = csp_hton32(seq++);
		fill_pattern(pp->data, data_size);

		packet->length = sizeof(*pp) + data_size;
		if (!csp_send(conn, packet, timeout_ms)) {
			csp_buffer_free(packet);
			break;
		}

		usleep(sleep_time);
	}

	printf("Sent %u packets\n", seq);

	return 0;
}

void sighandler(int signo)
{
	if (signo == SIGINT)
		should_stop = true;
}

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

int main(int argc, char **argv)
{
	int ret, c;
	int option_index = 0;
	struct csp_can_config conf = {.ifc = "vcan0"};

	uint8_t addr = 8;
	uint8_t port = 21;
	uint8_t server = 9;
	int server_mode = 0;
	int client_mode = 0;

	struct option long_options[] = {
		{"address",   required_argument, NULL,        'a'},
		{"bandwidth", required_argument, NULL,        'b'},
		{"client",    required_argument, NULL,        'c'},
		{"port",      required_argument, NULL,        'p'},
		{"timeout",   required_argument, NULL,        'T'},
		{"time",      required_argument, NULL,        't'},
		{"help",      no_argument,       NULL,        'h'},
		{"server",    no_argument,       &server_mode, 1},
	};

	while ((c = getopt_long(argc, argv, "a:b:c:p:st:T:h",
				long_options, &option_index)) != -1) {
		switch (c) {
		case 'a':
			addr = atoi(optarg);
			break;
		case 'b':
			bandwidth = atoi(optarg);
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
		case 't':
			runtime = atoi(optarg);
			break;
		case 'T':
			timeout_ms = atoi(optarg);
			break;
		case 'h':
			help(argv[0]);
			exit(EXIT_SUCCESS);
		case '?':
		default:
			exit(EXIT_FAILURE);
		}
	}

	/* Server XOR client must be selected */
	if (!(server_mode ^ client_mode)) {
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	csp_buffer_init(100, 512);
	csp_init(addr);

	csp_can_init(CSP_CAN_MASKED, &conf);
	csp_route_set(CSP_DEFAULT_ROUTE, &csp_if_can, CSP_NODE_MAC);

	csp_route_start_task(0, 0);

	/* Install SIGINT handler */
	signal(SIGINT, sighandler);

	if (server_mode) {
		ret = run_server(port);
	} else {
		ret = run_client(server, port);
	}

	return ret ? EXIT_FAILURE : EXIT_SUCCESS;
}
