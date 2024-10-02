#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include <csp/csp_id.h>
#include <csp/arch/csp_time.h>
#include <csp/interfaces/csp_if_eth_pbuf.h>

/* Buffer element timeout in ms */
#define PBUF_TIMEOUT_MS 1000

void csp_eth_pbuf_free(csp_eth_interface_data_t * ifdata, csp_packet_t * buffer, int buf_free, int * task_woken) {

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
		}

		prev = packet;
		packet = packet->next;
	}

}

csp_packet_t * csp_eth_pbuf_new(csp_eth_interface_data_t * ifdata, uint32_t id, uint32_t now, int * task_woken) {

	csp_eth_pbuf_cleanup(ifdata, now, task_woken);

	csp_packet_t * packet = (task_woken) ? csp_buffer_get_isr(0) : csp_buffer_get(0);
	if (packet == NULL) {
		return NULL;
	}

	packet->last_used = now;
	packet->cfpid = id;
	packet->frame_length = 0;

	/* Insert at beginning, because easy */
	packet->next = ifdata->pbufs;
	ifdata->pbufs = packet;

	return packet;
}

void csp_eth_pbuf_cleanup(csp_eth_interface_data_t * ifdata, uint32_t now, int * task_woken) {

	csp_packet_t * packet = ifdata->pbufs;
	csp_packet_t * prev = NULL;

	while (packet) {

		/* Perform cleanup in used pbufs */
		if ((now - packet->last_used) > PBUF_TIMEOUT_MS) {

			/* Erase from list prev->next = next */
			if (prev) {
				prev->next = packet->next;
			} else {
				ifdata->pbufs = packet->next;
			}

			if (task_woken == NULL) {
				csp_buffer_free(packet);
			} else {
				csp_buffer_free_isr(packet);
			}

		}

		prev = packet;
		packet = packet->next;
	}

}

csp_packet_t * csp_eth_pbuf_find(csp_eth_interface_data_t * ifdata, uint32_t id, int * task_woken) {

	uint32_t now = (task_woken) ? csp_get_ms_isr() : csp_get_ms();

	csp_packet_t * packet = ifdata->pbufs;
	while (packet) {

		if (packet->cfpid == id) {
			packet->last_used = now;
			return packet;
		}
		packet = packet->next;
	}

	return csp_eth_pbuf_new(ifdata, id, now, task_woken);

}
