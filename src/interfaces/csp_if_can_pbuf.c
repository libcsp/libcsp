#include "csp_if_can_pbuf.h"

#include <string.h>

#include <csp/csp_buffer.h>
#include <csp/csp_error.h>
#include <csp/arch/csp_time.h>

/* Buffer element timeout in ms */
#define PBUF_TIMEOUT_MS 1000

#if 1

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

	uint32_t now = (task_woken) ? csp_get_ms_isr() : csp_get_ms();

	csp_packet_t * packet = (task_woken) ? csp_buffer_get_isr(0) : csp_buffer_get(0);
	if (packet == NULL) {
		return NULL;
	}

	memset(packet->header, 0, sizeof(packet->header));
	packet->last_used = now;

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

#else

/* Number of packet buffer elements */
#define PBUF_ELEMENTS 20



static csp_can_pbuf_element_t csp_can_pbuf[PBUF_ELEMENTS] = {};

int csp_can_pbuf_free(csp_can_pbuf_element_t * buf, int * task_woken) {
	/* Free CSP packet */
	if (buf->packet != NULL) {
		if (task_woken == NULL) {
			csp_buffer_free(buf->packet);
		} else {
			csp_buffer_free_isr(buf->packet);
		}
	}

	/* Mark buffer element free */
	buf->packet = NULL;
	buf->rx_count = 0;
	buf->cfpid = 0;
	buf->last_used = 0;
	buf->remain = 0;
	buf->state = BUF_FREE;

	return CSP_ERR_NONE;
}

csp_can_pbuf_element_t * csp_can_pbuf_new(uint32_t id, int * task_woken) {
	uint32_t now = (task_woken) ? csp_get_ms_isr() : csp_get_ms();

	for (int i = 0; i < PBUF_ELEMENTS; i++) {

		/* Perform cleanup in used pbufs */
		if (csp_can_pbuf[i].state == BUF_USED) {
			if (now - csp_can_pbuf[i].last_used > PBUF_TIMEOUT_MS)
				csp_can_pbuf_free(&csp_can_pbuf[i], task_woken);
		}

		if (csp_can_pbuf[i].state == BUF_FREE) {
			csp_can_pbuf[i].state = BUF_USED;
			csp_can_pbuf[i].cfpid = id;
			csp_can_pbuf[i].remain = 0;
			csp_can_pbuf[i].last_used = now;
			return &csp_can_pbuf[i];
		}
	}

	return NULL;
}

csp_can_pbuf_element_t * csp_can_pbuf_find(uint32_t id, uint32_t mask, int * task_woken) {
	for (int i = 0; i < PBUF_ELEMENTS; i++) {

		if (csp_can_pbuf[i].state != BUF_USED)
			continue;

		if ((csp_can_pbuf[i].cfpid & mask) == (id & mask)) {
			csp_can_pbuf[i].last_used = (task_woken) ? csp_get_ms_isr() : csp_get_ms();
			return &csp_can_pbuf[i];
		}
	}
	return NULL;
}


#endif