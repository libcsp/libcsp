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

#ifndef LIB_CSP_SRC_INTERFACES_CSP_IF_CAN_PBUF_H_
#define LIB_CSP_SRC_INTERFACES_CSP_IF_CAN_PBUF_H_

#include <csp/csp_platform.h>

/* Packet buffers */
typedef enum {
	BUF_FREE = 0,			/* Buffer element free */
	BUF_USED = 1,			/* Buffer element used */
} csp_can_pbuf_state_t;

typedef struct {
	uint16_t rx_count;		/* Received bytes */
	uint32_t remain;		/* Remaining packets */
	uint32_t cfpid;			/* Connection CFP identification number */
	csp_packet_t *packet;		/* Pointer to packet buffer */
	csp_can_pbuf_state_t state;	/* Element state */
	uint32_t last_used;		/* Timestamp in ms for last use of buffer */
} csp_can_pbuf_element_t;

int csp_can_pbuf_free(csp_can_pbuf_element_t *buf, CSP_BASE_TYPE *task_woken);
csp_can_pbuf_element_t *csp_can_pbuf_new(uint32_t id, CSP_BASE_TYPE *task_woken);
csp_can_pbuf_element_t *csp_can_pbuf_find(uint32_t id, uint32_t mask, CSP_BASE_TYPE *task_woken);
void csp_can_pbuf_cleanup(CSP_BASE_TYPE *task_woken);

#endif
