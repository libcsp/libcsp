/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2012 Gomspace ApS (http://www.gomspace.com)
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

#include "csp_promisc.h"

#include <csp/csp.h>
#include <csp/arch/csp_queue.h>

#if (CSP_USE_PROMISC)

static csp_queue_handle_t csp_promisc_queue = NULL;
static int csp_promisc_enabled = 0;

int csp_promisc_enable(unsigned int queue_size) {

	/* If queue already initialised */
	if (csp_promisc_queue != NULL) {
		csp_promisc_enabled = 1;
		return CSP_ERR_NONE;
	}

	/* Create packet queue */
	csp_promisc_queue = csp_queue_create(queue_size, sizeof(csp_packet_t *));

	if (csp_promisc_queue == NULL)
		return CSP_ERR_INVAL;

	csp_promisc_enabled = 1;
	return CSP_ERR_NONE;

}

void csp_promisc_disable(void) {
	csp_promisc_enabled = 0;
}

csp_packet_t * csp_promisc_read(uint32_t timeout) {

	if (csp_promisc_queue == NULL)
		return NULL;

	csp_packet_t * packet = NULL;
	csp_queue_dequeue(csp_promisc_queue, &packet, timeout);

	return packet;

}

void csp_promisc_add(csp_packet_t * packet) {

	if (csp_promisc_enabled == 0)
		return;

	if (csp_promisc_queue != NULL) {
		/* Make a copy of the message and queue it to the promiscuous task */
		csp_packet_t *packet_copy = csp_buffer_clone(packet);
		if (packet_copy != NULL) {
			if (csp_queue_enqueue(csp_promisc_queue, &packet_copy, 0) != CSP_QUEUE_OK) {
				csp_log_error("Promiscuous mode input queue full");
				csp_buffer_free(packet_copy);
			}
		}
	}

}

#endif
