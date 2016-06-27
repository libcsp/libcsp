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
#include <inttypes.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

#include <csp/csp.h>
#include <csp/csp_endian.h>
#include <csp/csp_perf.h>
#include <csp/arch/csp_thread.h>
#include <csp/arch/csp_time.h>

#define CSPERF_TYPE_DATA 1
#define CSPERF_TYPE_DONE 2

struct csperf_packet {
	uint32_t seq;
	uint8_t type;
	uint8_t data[0];
} __attribute__ ((packed));

#if 0
static uint64_t get_us(void)
{
       struct timespec now;

       clock_gettime(CLOCK_MONOTONIC, &now);

       return now.tv_sec * 1000000UL + now.tv_nsec / 1000;
}
#else
static uint64_t get_us(void)
{
	return 0;
}
#endif


static void fill_pattern(uint8_t *dst, size_t len)
{
	size_t i;
	uint8_t b = 0x01;

	for (i = 0; i < len; i++)
		*dst++ = b++;
}

static bool check_pattern(uint8_t *dst, size_t len)
{
	size_t i;
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
	char unit = '\0';

	if (bytes > 1000000000) {
		bbytes = bytes / 1000000000.0;
		unit = 'G';
	} else if (bytes > 1000000) {
		bbytes = bytes / 1000000.0;
		unit = 'M';
	} else if (bytes > 1000) {
		bbytes = bytes / 1000.0;
		unit = 'K';
	} else {
		bbytes = bytes;
	}

	sprintf(out, "%5.1f %c", bbytes, unit);
}

struct csperf_state {
	unsigned int bytes;
	unsigned int received;
	unsigned int lost;
	unsigned int corrupt;
};

static void pretty_progress(unsigned int id,
			    unsigned int start,
			    unsigned int last,
			    uint32_t now,
			    struct csperf_state state)
{
	char bytestr[10];
	char bitstr[10];
	float diff_start = (last - start) / 1000.0;
	float diff_last = (now - start) / 1000.0;

	pretty_bytes(state.bytes, bytestr);
	pretty_bytes(state.bytes * 8 / (diff_last - diff_start), bitstr);
	printf("[%u] %4.1f-%4.1f sec  %sbytes  %sbit/s  %u rcv / %u lst / %u cpt\n",
	       id, diff_start, diff_last, bytestr, bitstr,
	       state.received, state.lost, state.corrupt);
}

static void handle_connection(unsigned int id, struct csp_perf_config *conf, csp_conn_t *conn)
{
	csp_packet_t *packet;
	struct csperf_packet *pp;

	struct csperf_state total = {0}, last = {0};

	uint32_t seq;
	uint32_t last_seq = 0;

	uint32_t now = 0;
	uint32_t last_start_time = 0;
	uint32_t start_time;

	printf("[%u] connected with %hhu port %hhu, flags %02x\n", id,
	       csp_conn_src(conn), csp_conn_sport(conn), csp_conn_flags(conn));

	start_time = csp_get_ms();
	last_start_time = start_time;
	while (1) {
		packet = csp_read(conn, conf->timeout_ms);
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
		if (now >= last_start_time + 1000 * conf->update_interval) {
			total.bytes += last.bytes;
			total.received += last.received;
			total.lost += last.lost;
			total.corrupt += last.corrupt;

			pretty_progress(id, start_time, last_start_time, now, last);
			last_start_time = now;

			memset(&last, 0, sizeof(last));
		}
	}

	total.bytes += last.bytes;
	total.received += last.received;
	total.lost += last.lost;
	total.corrupt += last.corrupt;

	pretty_progress(id, start_time, start_time, now, total);
}

int csp_perf_server(struct csp_perf_config *conf)
{
	csp_socket_t *socket;
	csp_conn_t *conn;
	unsigned int id = 1;

	socket = csp_socket(CSP_SO_NONE);
	if (!socket)
		return CSP_ERR_NOBUFS;

	csp_bind(socket, conf->port);
	csp_listen(socket, 10);

	printf("------------------------------------------------------------\n");
	printf("Server listening on port %hhu\n", conf->port);
	printf("------------------------------------------------------------\n");

	while (1) {
		if ((conn = csp_accept(socket, 250)) == NULL) {
			if (conf->should_stop)
				break;
			continue;
		}

		handle_connection(id++, conf, conn);

		csp_close(conn);
	}

	return 0;
}

int csp_perf_client(struct csp_perf_config *conf)
{
	csp_conn_t *conn;
	csp_packet_t *packet;
	struct csperf_packet *pp;

	uint32_t seq = 0;
	uint64_t start_time, transmit_time;
	int64_t interval_target;

	int64_t interval = 0, adjust = 0;

	printf("------------------------------------------------------------\n");
	printf("Client connecting to %hhu, port %hhu\n", conf->server, conf->port);
	printf("------------------------------------------------------------\n");

	conn = csp_connect(CSP_PRIO_NORM, conf->server, conf->port, conf->timeout_ms, conf->flags);
	if (!conn) {
		printf("Failed to connect to server\n");
		return -ENOTCONN;
	}

	/* Calculate target time between packets in us for target speed */
	if (conf->bandwidth > 0)
		interval_target = (sizeof(*pp) + conf->data_size) * 8 * 1000000 / conf->bandwidth;
	else
		interval_target = 0;
	printf("%"PRIi64" us between packets\n", interval_target);

	start_time = get_us();

	while (1) {
		/* Log start of transmission cycle */
		transmit_time = get_us();

		int64_t sleep_time = interval_target - adjust;
		if (sleep_time < 0)
			sleep_time = 0;

		if (conf->should_stop)
			break;

		if (get_us() > start_time + conf->runtime * 1000000U)
			break;

		packet = csp_buffer_get(sizeof(*pp) + conf->data_size);
		if (!packet) {
			printf("Failed to allocate packet\n");
			return -ENOMEM;
		}

		pp = (struct csperf_packet *) packet->data;

		pp->seq = csp_hton32(seq++);
		fill_pattern(pp->data, conf->data_size);

		packet->length = sizeof(*pp) + conf->data_size;
		if (!csp_send(conn, packet, conf->timeout_ms)) {
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

	printf("Sent %"PRIu32" packets\n", seq);

	return 0;
}
