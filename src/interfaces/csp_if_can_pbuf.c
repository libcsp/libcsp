/*
 * csp_if_can_pbuf.c
 *
 *  Created on: Feb 3, 2017
 *      Author: johan
 */

#include <csp/arch/csp_time.h>
#include "csp_if_can_pbuf.h"

/* Number of packet buffer elements */
#define PBUF_ELEMENTS		CSP_CONN_MAX

/* Buffer element timeout in ms */
#define PBUF_TIMEOUT_MS		1000

static csp_can_pbuf_element_t csp_can_pbuf[PBUF_ELEMENTS] = {};

int csp_can_pbuf_free(csp_can_pbuf_element_t *buf, CSP_BASE_TYPE *task_woken)
{
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

csp_can_pbuf_element_t *csp_can_pbuf_new(uint32_t id, CSP_BASE_TYPE *task_woken)
{
	uint32_t now = csp_get_ms();

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

csp_can_pbuf_element_t *csp_can_pbuf_find(uint32_t id, uint32_t mask)
{
	for (int i = 0; i < PBUF_ELEMENTS; i++) {
		if ((csp_can_pbuf[i].state == BUF_USED) && ((csp_can_pbuf[i].cfpid & mask) == (id & mask))) {
			csp_can_pbuf[i].last_used = csp_get_ms();
			return &csp_can_pbuf[i];
		}
	}
	return NULL;
}

