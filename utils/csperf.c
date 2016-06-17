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
#include <csp/arch/csp_thread.h>
#include <csp/arch/csp_time.h>

unsigned int timeout_ms = 1000;
unsigned int data_size = 100;
bool should_stop = false;
unsigned int runtime = 10;
unsigned int max_frames = 0;
unsigned int bandwidth = 1000000; /* bits/s */
unsigned int update_interval = 1;

#define CSPERF_TYPE_DATA 1
#define CSPERF_TYPE_DONE 2

struct csperf_packet {
	uint32_t seq;
	uint8_t type;
	uint8_t data[0];
} __attribute__ ((packed));

static uint64_t get_us(void)
{
       struct timespec now;

       clock_gettime(CLOCK_MONOTONIC, &now);

       return now.tv_sec * 1000000UL + now.tv_nsec / 1000;
}

static void fill_pattern(uint8_t *dst, size_t len)
{
	int i;
	uint8_t b = 0x01;

	for (i = 0; i < len; i++)
		*dst++ = b++;
}

static bool check_pattern(uint8_t *dst, size_t len)
{
	int i;
	uint8_t b = 0x01;

	for (i = 0; i < len; i++) {
		if (*dst++ != b++)
			return false;
	}

	return true;
}

static void pretty_bytes(uint32_t bytes, char *out)
{
	float bbytes;
	char *unit = "";

	if (bytes > 1000000000) {
		bbytes = bytes / 1000000000.0;
		unit = "G";
	} else if (bytes > 1000000) {
		bbytes = bytes / 1000000.0;
		unit = "M";
	} else if (bytes > 1000) {
		bbytes = bytes / 1000.0;
		unit = "K";
	} else {
		bbytes = bytes;
	}

	sprintf(out, "%5.1f%s", bbytes, unit);
}

struct csperf_state {
	uint32_t time;

	uint32_t bytes;
	uint32_t received;
	uint32_t lost;
	uint32_t corrupt;
};

static void pretty_progress(uint32_t start, uint32_t last, uint32_t now, struct csperf_state state)
{
	char bytestr[10];
	char bitstr[10];
	float diff_start = (last - start) / 1000.0;
	float diff_last = (now - start) / 1000.0;

	pretty_bytes(state.bytes, bytestr);
	pretty_bytes(state.bytes * 8 / (diff_last - diff_start), bitstr);
	printf("[ ] %.1f - %.1f  %s  %sbit/s %u/%u/%u\n",
	       diff_start, diff_last, bytestr, bitstr, state.received, state.lost, state.corrupt);
}

static void handle_connection(csp_conn_t *conn)
{
	csp_packet_t *packet;
	struct csperf_packet *pp;

	struct csperf_state total = {0}, last = {0};

	uint32_t seq;
	uint32_t last_seq = 0;

	uint32_t now = 0;
	uint32_t last_start_time = 0;
	uint32_t start_time;

	printf("[ ] connected with %hhu port %hhu, flags %02x\n",
	       csp_conn_src(conn), csp_conn_sport(conn), csp_conn_flags(conn));

	start_time = csp_get_ms();
	last_start_time = start_time;
	while (1) {
		packet = csp_read(conn, timeout_ms);
		if (!packet)
			break;

		pp = (struct csperf_packet *) packet->data;
		seq = csp_ntoh32(pp->seq);

		/* Update counters */
		last.received += 1;
		last.bytes += packet->length;

		if (last_seq > 0 && seq != last_seq + 1)
			last.lost += seq - last_seq - 1;

		if (!check_pattern(pp->data, packet->length - sizeof(*pp)))
			last.corrupt++;

		last_seq = seq;

		csp_buffer_free(packet);

		/* Update progress */
		now = csp_get_ms();
		if (now >= last_start_time + 1000 * update_interval) {
			total.bytes += last.bytes;
			total.received += last.received;
			total.lost += last.lost;
			total.corrupt += last.corrupt;

			pretty_progress(start_time, last_start_time, now, last);
			last_start_time = now;

			memset(&last, 0, sizeof(last));
		}
	}

	total.bytes += last.bytes;
	total.received += last.received;
	total.lost += last.lost;
	total.corrupt += last.corrupt;

	pretty_progress(start_time, start_time, now, total);
}

static int run_server(uint8_t port)
{
	csp_socket_t *socket;
	csp_conn_t *conn;

	socket = csp_socket(CSP_SO_NONE);
	assert(socket);
	csp_bind(socket, port);
	csp_listen(socket, 10);

	printf("------------------------------------------------------------\n");
	printf("Server listening on port %hhu\n", port);
	printf("------------------------------------------------------------\n");

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

static int run_client(uint8_t server, uint8_t port, uint8_t flags)
{
	csp_conn_t *conn;
	csp_packet_t *packet;
	struct csperf_packet *pp;

	uint32_t seq = 0;
	uint64_t start_time, transmit_time;
	int64_t interval_target;

	int64_t interval = 0, adjust = 0;

	printf("------------------------------------------------------------\n");
	printf("Client connecting to %hhu, port %hhu\n", server, port);
	printf("------------------------------------------------------------\n");

	conn = csp_connect(CSP_PRIO_NORM, server, port, timeout_ms, flags);
	if (!conn) {
		printf("Failed to connect to server\n");
		return -ENOTCONN;
	}

	/* Calculate target time between packets in us for target speed */
	if (bandwidth > 0)
		interval_target = (sizeof(*pp) + data_size) * 8 * 1000000 / bandwidth;
	else
		interval_target = 0;
	printf("%lu us between packets\n", interval_target);

	start_time = get_us();

	while (1) {
		/* Log start of transmission cycle */
		transmit_time = get_us();

		int64_t sleep_time = interval_target - adjust;
		if (sleep_time < 0)
			sleep_time = 0;

		if (should_stop)
			break;

		if (get_us() > start_time + runtime * 1000000U)
			break;

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

		/* Sleep until next transmission */
		usleep(sleep_time);

		interval = (int64_t)get_us() - (int64_t)transmit_time;

		/* Adjust next sleep with the time we actually slept last
		 * cycle. Correct with the adjustment value from last cycle to
		 * avoid double compensation */
		adjust = (interval - interval_target) + adjust;

	}

	printf("Sent %u packets\n", seq);

	return 0;
}

static void sighandler(int signo)
{
	if (signo == SIGINT)
		should_stop = true;
}

static void usage(const char *argv0)
{
	printf("usage: %s [-s|-c host]\n", argv0);
}

static void help(const char *argv0)
{
	usage(argv0);
	printf("  -a, --address ADDRESS    Set address\r\n");
	printf("  -s, --server             Run in server mode\n");
	printf("  -h, --help               Print help and exit\r\n");
}

static uint8_t parse_flags(const char *flagstr)
{
	uint8_t flags = CSP_O_NONE;

	if (strchr(flagstr, 'r'))
		flags |= CSP_O_RDP;
	if (strchr(flagstr, 'c'))
		flags |= CSP_O_CRC32;
	if (strchr(flagstr, 'h'))
		flags |= CSP_O_HMAC;
	if (strchr(flagstr, 'x'))
		flags |= CSP_O_XTEA;

	return flags;
}

int main(int argc, char **argv)
{
	int ret, c;
	int option_index = 0;
	struct csp_can_config conf = {.ifc = "vcan0"};

	uint8_t addr = 8;
	uint8_t port = 21;
	uint8_t server = 9;
	uint8_t flags = CSP_O_NONE;
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
		{"option",    required_argument, NULL,        'o'},
		{"size",      required_argument, NULL,        'z'},
		{"server",    no_argument,       &server_mode, 1},
	};

	while ((c = getopt_long(argc, argv, "a:b:c:o:p:st:T:hz:",
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
		case 'o':
			flags = parse_flags(optarg);
			break;
		case 'h':
			help(argv[0]);
			exit(EXIT_SUCCESS);
		case 'z':
			data_size = atoi(optarg);
			break;
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
		ret = run_client(server, port, flags);
	}

	return ret ? EXIT_FAILURE : EXIT_SUCCESS;
}
