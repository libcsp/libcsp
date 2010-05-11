/*
 * csp_service_handler.c
 *
 *  Created on: 03/05/2010
 *      Author: oem
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

/* CSP includes */
#include <csp/csp.h>
#include <csp/csp_endian.h>

#include "arch/csp_time.h"
#include "arch/csp_malloc.h"

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
		break;

	/* Retrieve the ProcessList as a string */
	case CSP_PS:
#if defined(_CSP_FREERTOS_)
		vTaskList((signed portCHAR *) packet->data);
		packet->length = strlen((char *)packet->data);
		packet->data[packet->length] = '\0';
		packet->length++;
#elif defined(_CSP_POSIX_)
        char * str = "Tasklist in not available on posix";
        strncpy(packet->data, str, strlen(str));
        packet->length = strlen(str);
#endif
        break;

	/* Do a search for the largest block of free memory */
	case CSP_MEMFREE: {

		/* Try to malloc a lot */
	    size_t size = 0;
#if defined(_CSP_FREERTOS_)
		void * pmem;
		while(1) {
			size = size + 100;
			pmem = csp_malloc(size);
			if (pmem == NULL) {
				size = size - 100;
				break;
			} else {
				csp_free(pmem);
			}
		}
#elif defined(_CSP_POSIX_)
        /* Use sysinfo for this */
        size = 1000;
#endif

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
		extern void __attribute__((weak)) cpu_reset(void);
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

	default:
		/* The connection was not a service-request, free the packet and return */
		csp_buffer_free(packet);
		return;
	}

	/* Try to send packet, by reusing the incoming buffer */
	if (!csp_send(conn, packet, 0))
		csp_buffer_free(packet);

}
