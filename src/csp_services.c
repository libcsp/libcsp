/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2010 GomSpace ApS (gomspace.com)
Copyright (C) 2010 AAUSAT3 Project (aausat3.space.aau.dk) 

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
#include <stdint.h>

/* CSP includes */
#include <csp/csp.h>
#include <csp/csp_endian.h>

#include "arch/csp_time.h"

void csp_ping(uint8_t node, unsigned int timeout) {

	uint32_t start, time, status = 0;

	printf("Ping node %u: ", node);

	/* Counter */
	start = csp_get_ms();

	/* Open connection */
	csp_conn_t * conn = csp_connect(CSP_PRIO_NORM, node, CSP_PING, timeout, CSP_O_RDP | CSP_O_XTEA | CSP_O_HMAC | CSP_O_CRC32);
	if (conn == NULL) {
		printf("Timeout!\r\n");
		return;
	}

	/* Prepare data */
	csp_packet_t * packet;
	packet = csp_buffer_get(1);

	/* Check malloc */
	if (packet == NULL)
		goto out;

	packet->data[0] = 0x55;
	packet->length = 1;

	/* Try to send frame */
	if (!csp_send(conn, packet, 0))
		goto out;

	/* Read incoming frame */
	packet = csp_read(conn, timeout);

	if (packet != NULL)
		status = 1;

out:
	/* Clean up */
	if (packet != NULL)
		csp_buffer_free(packet);
	csp_close(conn);

	/* We have a reply */
	time = (csp_get_ms() - start);

	if (status) {
		if (time <= 1) {
			printf("Reply in <1 tick\r\n");
		} else {
			printf("Reply in %u ms\r\n", (unsigned int) time);
		}
	} else {
		printf("Timeout!\r\n");
	}

}

void csp_ping_noreply(uint8_t node) {

	/* Prepare data */
	csp_packet_t * packet;
	packet = csp_buffer_get(1);

	/* Check malloc */
	if (packet == NULL)
		return;

	/* Open connection */
	csp_conn_t * conn = csp_connect(CSP_PRIO_NORM, node, CSP_PING, 0, 0);
	if (conn == NULL) {
		csp_buffer_free(packet);
		return;
	}

	packet->data[0] = 0x55;
	packet->length = 1;

	printf("Ping ignore reply node %u.\r\n", node);

	/* Try to send frame */
	if (!csp_send(conn, packet, 0))
		csp_buffer_free(packet);

	csp_close(conn);

}

void csp_reboot(uint8_t node) {
	uint32_t magic_word = htonl(0x80078007);
	csp_transaction(CSP_PRIO_NORM, node, CSP_REBOOT, 0, &magic_word, sizeof(magic_word), NULL, 0);
}

void csp_ps(uint8_t node, unsigned int timeout) {

	/* Open connection */
	csp_conn_t * conn = csp_connect(CSP_PRIO_NORM, node, CSP_PS, 0, 0);
	if (conn == NULL)
		return;

	/* Prepare data */
	csp_packet_t * packet;
	packet = csp_buffer_get(95);

	/* Check malloc */
	if (packet == NULL)
		goto out;

	packet->data[0] = 0x55;
	packet->length = 1;

	printf("PS node %u: ", node);

	/* Try to send frame */
	if (!csp_send(conn, packet, 0))
		goto out;

	/* Read incoming frame */
	packet = csp_read(conn, timeout);
	if (packet == NULL) {
		printf(" Timeout!\r\n");
		goto out;
	}

	/* We have a reply */
	printf("PS Length %u\r\n", packet->length);
	printf("%s\r\n", packet->data);

	/* Clean up */
out:
	if (packet != NULL)
		csp_buffer_free(packet);
	csp_close(conn);

}

void csp_memfree(uint8_t node, unsigned int timeout) {

	uint32_t memfree;

	int status = csp_transaction(CSP_PRIO_NORM, node, CSP_MEMFREE, timeout, NULL, 0, &memfree, sizeof(memfree));
	if (status == 0) {
		printf("Network error\r\n");
		return;
	}

	/* Convert from network to host order */
	memfree = ntohl(memfree);

	printf("Free Memory at node %u is %u bytes\r\n", node, (unsigned int) memfree);

}

void csp_buf_free(uint8_t node, unsigned int timeout) {

	uint32_t size = 0;

	int status = csp_transaction(CSP_PRIO_NORM, node, CSP_BUF_FREE, timeout, NULL, 0, &size, sizeof(size));
	if (status == 0) {
		printf("Network error\r\n");
		return;
	}
	size = ntohl(size);
	printf("Free buffers at node %u is %u\r\n", (unsigned int) node, (unsigned int) size);

}

void csp_uptime(uint8_t node, unsigned int timeout) {

	uint32_t uptime = 0;

	int status = csp_transaction(CSP_PRIO_NORM, node, CSP_UPTIME, timeout, NULL, 0, &uptime, sizeof(uptime));
	if (status == 0) {
		printf("Network error\r\n");
		return;
	}
	uptime = ntohl(uptime);
	printf("Uptime of node %u is %u s\r\n", (unsigned int) node, (unsigned int) uptime);

}
