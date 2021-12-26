#include "csp_if_can_pbuf.h"

#include <string.h>

#include <csp/csp_buffer.h>
#include <csp/csp_error.h>
#include <csp/arch/csp_time.h>

/* Buffer element timeout in ms */
#define PBUF_TIMEOUT_MS 1000

/* CAN  are stored in a linked list */
static csp_packet_t * buffers = NULL;

void csp_can_pbuf_free(csp_packet_t * buffer, int buf_free, int * task_woken) {

	csp_packet_t * packet = buffers;
	csp_packet_t * prev = NULL;

	while (packet) {

		/* Perform cleanup in used pbufs */
		if (packet == buffer) {

			/* Erase from list prev->next = next */
			if (prev) {
				prev->next = packet->next;
			} else {
				buffers = packet->next;
			}

			if (buf_free) {
				if (task_woken == NULL) {
					csp_buffer_free(packet);
				} else {
					csp_buffer_free_isr(packet);
				}
			}

		}

		prev = packet;
		packet = packet->next;
	}

}

csp_packet_t * csp_can_pbuf_new(uint32_t id, int * task_woken) {

	csp_can_pbuf_cleanup();

	uint32_t now = (task_woken) ? csp_get_ms_isr() : csp_get_ms();

	csp_packet_t * packet = (task_woken) ? csp_buffer_get_isr(0) : csp_buffer_get(0);
	if (packet == NULL) {
		return NULL;
	}

	packet->last_used = now;
	packet->cfpid = id;
	packet->remain = 0;

	/* Insert at beginning, because easy */
	packet->next = buffers;
	buffers = packet;

	return packet;
}

void csp_can_pbuf_cleanup(void) {

	uint32_t now = csp_get_ms();

	csp_packet_t * packet = buffers;
	csp_packet_t * prev = NULL;

	while (packet) {

		/* Perform cleanup in used pbufs */
		if (now - packet->last_used > PBUF_TIMEOUT_MS) {

			/* Erase from list prev->next = next */
			if (prev) {
				prev->next = packet->next;
			} else {
				buffers = packet->next;
			}

			csp_buffer_free(packet);
		}

		prev = packet;
		packet = packet->next;
	}

}

csp_packet_t * csp_can_pbuf_find(uint32_t id, uint32_t mask, int * task_woken) {

	csp_packet_t * packet = buffers;
	while (packet) {

		if ((packet->cfpid & mask) == (id & mask)) {
			packet->last_used = (task_woken) ? csp_get_ms_isr() : csp_get_ms();
			return packet;
		}
		packet = packet->next;
	}

	return NULL;

}
