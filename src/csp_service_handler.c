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
#include "arch/csp_malloc.h"

#ifdef _CSP_POSIX_
#include <sys/sysinfo.h>
#endif

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

	switch (csp_conn_dport(conn)) {

	/* A ping means, just echo the packet, so no changes */
	case CSP_PING:
		csp_debug(CSP_INFO, "SERVICE: Ping received\r\n");
		break;

	/* Retrieve the ProcessList as a string */
	case CSP_PS: {
#if defined(_CSP_FREERTOS_)
		vTaskList((signed portCHAR *) packet->data);
#elif defined(_CSP_POSIX_)
        strcpy((char *)packet->data, "Tasklist in not available on posix");
#endif
        packet->length = strlen((char *)packet->data);
        packet->data[packet->length] = '\0';
		packet->length++;
        break;
    }

	/* Do a search for the largest block of free memory */
	case CSP_MEMFREE: {

#if defined(_CSP_FREERTOS_)
		/* Try to malloc a lot */
		uint32_t size = 1000000, total = 0, max = UINT32_MAX;
		void * pmem;
		while (1) {
			pmem = csp_malloc(size + total);
			if (pmem == NULL) {
				max = size + total;
				size = size / 2;
			} else {
				total += size;
				if (total + size >= max)
					size = size / 2;
				csp_free(pmem);
			}
			if (size < 1024) break;
		}
#elif defined(_CSP_POSIX_)
		/* Read system statistics */
		size_t total = 0;
        struct sysinfo info;
        sysinfo(&info);
        total = info.freeram * info.mem_unit;
#endif

		/* Prepare for network transmission */
		total = htonl(total);
		memcpy(packet->data, &total, sizeof(total));
		packet->length = sizeof(total);

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
		extern void __attribute__((weak)) cpu_set_reset_cause(unsigned int);
		if (cpu_set_reset_cause)
			cpu_set_reset_cause(1);
		extern void __attribute__((weak)) cpu_reset(void);
		if (cpu_reset) {
			cpu_reset();
			while (1);
		}

		csp_buffer_free(packet);
		return;
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

	case CSP_UPTIME: {
		uint32_t time = csp_get_s();
		time = htonl(time);
		memcpy(packet->data, &time, sizeof(time));
		packet->length = sizeof(time);
		break;
	}

	default:
		/* The connection was not a service-request, free the packet and return */
		csp_buffer_free(packet);
		return;
	}

	/* Try to send packet, by reusing the incoming buffer */
	if (!csp_send(conn, packet, 0))
		csp_buffer_free(packet);

}
