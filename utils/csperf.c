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
#include <csp/csp_perf.h>
#include <csp/interfaces/csp_if_can.h>
#include <csp/arch/csp_thread.h>
#include <csp/arch/csp_time.h>

/* Must be global so signal can stop server */
static struct csp_perf_config perf_conf;

static void sighandler(int signo)
{
	if (signo == SIGINT)
		csp_perf_stop(&perf_conf);
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
	struct csp_can_config can_conf = {.ifc = "vcan0"};

	uint8_t addr = 8;
	int server_mode = 0;
	int client_mode = 0;

	struct option long_options[] = {
		{"address",   required_argument, NULL,        'a'},
		{"bandwidth", required_argument, NULL,        'b'},
		{"client",    required_argument, NULL,        'c'},
		{"port",      required_argument, NULL,        'p'},
		{"can",       required_argument, NULL,        'i'},
		{"timeout",   required_argument, NULL,        'T'},
		{"time",      required_argument, NULL,        't'},
		{"help",      no_argument,       NULL,        'h'},
		{"option",    required_argument, NULL,        'o'},
		{"size",      required_argument, NULL,        'z'},
		{"server",    no_argument,       &server_mode, 1},
	};

	csp_perf_set_defaults(&perf_conf);

	while ((c = getopt_long(argc, argv, "a:b:c:i:o:p:st:T:hz:",
				long_options, &option_index)) != -1) {
		switch (c) {
		case 'a':
			addr = atoi(optarg);
			break;
		case 'b':
			perf_conf.bandwidth = atoi(optarg);
			break;
		case 'c':
			client_mode = 1;
			perf_conf.server = atoi(optarg);
			break;
		case 'p':
			perf_conf.port = atoi(optarg);
			break;
		case 'i':
			can_conf.ifc = optarg;
			break;
		case 's':
			server_mode = 1;
			break;
		case 't':
			perf_conf.runtime = atoi(optarg);
			break;
		case 'T':
			perf_conf.timeout_ms = atoi(optarg);
			break;
		case 'o':
			perf_conf.flags = parse_flags(optarg);
			break;
		case 'h':
			help(argv[0]);
			exit(EXIT_SUCCESS);
		case 'z':
			perf_conf.data_size = atoi(optarg);
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

	csp_can_init(CSP_CAN_MASKED, &can_conf);
	csp_route_set(CSP_DEFAULT_ROUTE, &csp_if_can, CSP_NODE_MAC);

	csp_route_start_task(0, 0);

	/* Install SIGINT handler */
	signal(SIGINT, sighandler);

	if (server_mode) {
		ret = csp_perf_server(&perf_conf);
	} else {
		ret = csp_perf_client(&perf_conf);
	}

	return ret ? EXIT_FAILURE : EXIT_SUCCESS;
}
