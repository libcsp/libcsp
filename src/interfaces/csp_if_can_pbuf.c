/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2012 GomSpace ApS (http://www.gomspace.com)
Copyright (C) 2012 AAUSAT3 Project (http://aausat3.space.aau.dk)

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

#include "csp_if_can_pbuf.h"

#include <csp/csp_buffer.h>
#include <csp/csp_error.h>
#include <csp/arch/csp_time.h>

/* Number of packet buffer elements */
#define PBUF_ELEMENTS		5

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

csp_can_pbuf_element_t *csp_can_pbuf_find(uint32_t id, uint32_t mask, CSP_BASE_TYPE *task_woken)
{
	for (int i = 0; i < PBUF_ELEMENTS; i++) {
		if ((csp_can_pbuf[i].state == BUF_USED) && ((csp_can_pbuf[i].cfpid & mask) == (id & mask))) {
			csp_can_pbuf[i].last_used = (task_woken) ? csp_get_ms_isr() : csp_get_ms();
			return &csp_can_pbuf[i];
		}
	}
	return NULL;
}

