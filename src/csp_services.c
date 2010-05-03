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
#include <csp/csp_time.h>

//#include <dev/cpu.h>
//#include <util/hton.h>

/**
 * If the given packet is a service-request (that is uses one of the csp service ports)
 * it will be handled according to the CSP service handler.
 * This function will either use the packet buffer or delete it,
 * so this function is typically called in the last "default" clause of
 * a switch/case statement in a csp_listener task.
 * In order to listen to csp service ports, bind your listener to the CSP_ANY port.
 * This function may only be called from task context.
 * @param conn Pointer to the new connection
 * @param packet Pointer to the first packet, obtained by using csp_read()
 */
void csp_service_handler(csp_conn_t * conn, csp_packet_t * packet) {

	switch (conn->idin.dport) {

	/* A ping means, just echo the packet, so no changes */
	case CSP_PING:
		break;

/* We need to find a good solution to this */
#if 0
	/* Retrieve the ProcessList as a string */
	case CSP_PS:
		vTaskList((signed portCHAR *) packet->data);
		packet->length = strlen((char *)packet->data);
		packet->data[packet->length] = '\0';
		packet->length++;
		break;

	/* Do a search for the largest block of free memory */
	case CSP_MEMFREE: {

		/* Try to malloc a lot */
		uint32_t size = 0;
		void * pmem;
		while(1) {
			size = size + 100;
			pmem = pvPortMallocFromISR(size);
			if (pmem == NULL) {
				size = size - 100;
				break;
			} else {
				vPortFreeFromISR(pmem);
			}
		}

		/* Prepare for network transmission */
		size = htonl(size);
		memcpy(packet->data, &size, sizeof(size));
		packet->length = sizeof(size);

		break;
	}

	/* Call this port with the magic word to reboot */
	case CSP_REBOOT: {
		uint32_t magic_word;
		memcpy(&magic_word, packet->data, sizeof(magic_word));

		magic_word = ntohl(magic_word);

		/* If the magic word is invalid, return */
		if (magic_word != 0x80078007) {
			csp_buffer_free(packet);
			return;
		}

		/* Otherwise Reboot */
		cpu_reset();
		break;
	}

	/* Return the number of free CSP buffers */
	case CSP_BUF_FREE: {
		uint32_t size = csp_buffer_remaining();
		/* Prepare for network transmission */
		size = htonl(size);
		memcpy(packet->data, &size, sizeof(size));
		packet->length = sizeof(size);
		break;
	}
#endif

	default:
		/* The connection was not a service-request, free the packet and return */
		csp_buffer_free(packet);
		return;
	}

	/* Try to send packet, by reusing the incoming buffer */
	if (!csp_send(conn, packet, 0))
		csp_buffer_free(packet);

}

void csp_ping(uint8_t node, int timeout) {

	/* Prepare data */
	csp_packet_t * packet;
	packet = csp_buffer_get(sizeof(csp_packet_t));

	/* Check malloc */
	if (packet == NULL)
		return;

	packet->data[0] = 0x55;
	packet->length = 1;

	/* Open connection */
	csp_conn_t * conn = csp_connect(PRIO_NORM, node, CSP_PING);
	printf("Ping node %u: ", node);

	/* Counter */
    uint32_t start = csp_get_ms();
	//portTickType start = xTaskGetTickCount();

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
	//portTickType time = (xTaskGetTickCount() - start);
    uint32_t time = (csp_get_ms() - start);
	if (time <= 1) {
		printf(" Reply in <1 tick\r\n");
	} else {
		printf(" Reply in %u ms\r\n", (unsigned int) time);
//		printf(" Reply in <%u ms (1 tick)\r\n", (unsigned int) (1000/configTICK_RATE_HZ));
//	} else {
//		printf(" Reply in %u ms\r\n", (unsigned int) (time * (1000/configTICK_RATE_HZ)));
	}

	/* Clean up */
out:
	if (packet != NULL)
		csp_buffer_free(packet);
	csp_close(conn);

}

void csp_ping_noreply(uint8_t node) {

	/* Prepare data */
	csp_packet_t * packet;
	packet = csp_buffer_get(sizeof(csp_packet_t));

	/* Check malloc */
	if (packet == NULL)
		return;

	packet->data[0] = 0x55;
	packet->length = 1;

	/* Open connection */
	csp_conn_t * conn = csp_connect(PRIO_NORM, node, CSP_PING);
	printf("Ping ignore reply node %u.\r\n", node);

	/* Try to send frame */
	if (!csp_send(conn, packet, 0))
		csp_buffer_free(packet);

	csp_close(conn);

}

/* Does these functions belong here? */

#if 0
void csp_reboot(uint8_t node) {
	uint32_t magic_word = htonl(0x80078007);
	csp_transaction(PRIO_NORM, node, CSP_REBOOT, 0, &magic_word, sizeof(magic_word), NULL, 0);
}

void csp_ps(uint8_t node, portTickType timeout) {

	/* Prepare data */
	csp_packet_t * packet;
	packet = csp_buffer_get(sizeof(csp_packet_t));

	/* Check malloc */
	if (packet == NULL)
		return;

	packet->data[0] = 0x55;
	packet->length = 1;

	/* Open connection */
	conn_t * conn = csp_connect(PRIO_NORM, node, CSP_PS);
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

void csp_memfree(uint8_t node, portTickType timeout) {

	uint32_t memfree;

	int status = csp_transaction(PRIO_NORM, node, CSP_MEMFREE, timeout, NULL, 0, &memfree, sizeof(memfree));
	if (status == 0) {
		printf("Network error\r\n");
		return;
	}

	/* Convert from network to host order */
	memfree = ntohl(memfree);

	printf("Free Memory at node %u is %u bytes\r\n", node, (unsigned int) memfree);

}

void csp_buf_free(uint8_t node, portTickType timeout) {

	uint32_t size = 0;

	int status = csp_transaction(PRIO_NORM, node, CSP_BUF_FREE, timeout, NULL, 0, &size, sizeof(size));
	if (status == 0) {
		printf("Network error\r\n");
		return;
	}
	size = ntohl(size);
	printf("Free buffers at node %u is %u\r\n", (unsigned int) node, (unsigned int) size);

}
#endif
