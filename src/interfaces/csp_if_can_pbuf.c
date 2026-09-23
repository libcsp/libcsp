#include "csp_if_can_pbuf.h"

#include <string.h>

#include <csp/csp_buffer.h>
#include <csp/csp_error.h>
#include <csp/arch/csp_time.h>
#include <csp/interfaces/csp_if_can.h>
#include <csp/csp_iflist.h>

#include "../csp_buffer_private.h"

/* Buffer element timeout in ms */
#define PBUF_TIMEOUT_MS CSP_CAN_PBUF_TIMEOUT_MS

void csp_can_pbuf_free(csp_can_interface_data_t * ifdata, csp_packet_t * buffer, int buf_free, int * task_woken) {

	csp_packet_t * packet = ifdata->pbufs;
	csp_packet_t * prev = NULL;

	while (packet) {

		/* Perform cleanup in used pbufs */
		if (packet == buffer) {

			/* Erase from list prev->next = next */
			if (prev) {
				prev->next = packet->next;
			} else {
				ifdata->pbufs = packet->next;
			}

			if (buf_free) {
				if (task_woken == NULL) {
					csp_buffer_free(packet);
				} else {
					csp_buffer_free_isr(packet);
				}
			}
			return;
		}

		prev = packet;
		packet = packet->next;
	}

}

csp_packet_t * csp_can_pbuf_new(csp_can_interface_data_t * ifdata, uint32_t id, csp_id_t csp_id, int * task_woken) {

	csp_can_pbuf_cleanup(ifdata, task_woken);

	uint32_t now = (task_woken) ? csp_get_ms_isr() : csp_get_ms();

	csp_packet_t * packet = NULL;
	if (csp_iflist_get_by_addr(csp_id.dst) != NULL) {
		/* The packet is for us, make sure we don't silently ignore the situation if we can't process it */
		packet = (task_woken) ? csp_buffer_get_always_isr() : csp_buffer_get_always();
	} else  {
		/* The packet is not for us, it is ok to drop it if we don't have enough buffers*/
		packet = (task_woken) ? csp_buffer_get_isr(0) : csp_buffer_get(0);
	}

	if (packet == NULL) {
		return packet;
	}

	packet->last_used = now;
	packet->cfpid = id;
	packet->remain = 0;
#if (CSP_CFP_OUT_OF_ORDER_RX)
	memset(packet->cfp_rx_bitmap, 0, sizeof(packet->cfp_rx_bitmap));
#endif

	/* Insert at beginning, because easy */
	packet->next = ifdata->pbufs;
	ifdata->pbufs = packet;

	return packet;
}

void csp_can_pbuf_cleanup(csp_can_interface_data_t * ifdata, int * task_woken) {

	uint32_t now = (task_woken) ? csp_get_ms_isr() : csp_get_ms();

	csp_packet_t * packet = ifdata->pbufs;
	csp_packet_t * prev = NULL;

	while (packet) {

		csp_packet_t * next = packet->next;

		/* Perform cleanup in used pbufs */
		if (now - packet->last_used > PBUF_TIMEOUT_MS) {

			/* Erase from list prev->next = next */
			if (prev) {
				prev->next = next;
			} else {
				ifdata->pbufs = next;
			}

			if (task_woken == NULL) {
				csp_buffer_free(packet);
			} else {
				csp_buffer_free_isr(packet);
			}

			packet = next;
			continue;

		}

		prev = packet;
		packet = next;
	}

}

csp_packet_t * csp_can_pbuf_find(csp_can_interface_data_t * ifdata, uint32_t id, uint32_t mask, int * task_woken) {

	csp_packet_t * packet = ifdata->pbufs;
	while (packet) {

		if ((packet->cfpid & mask) == (id & mask)) {
			packet->last_used = (task_woken) ? csp_get_ms_isr() : csp_get_ms();
			return packet;
		}
		packet = packet->next;
	}

	return NULL;

}
