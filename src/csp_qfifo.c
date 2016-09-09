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

#include <csp/csp.h>
#include <csp/csp_interface.h>
#include <csp/arch/csp_queue.h>
#include "csp_qfifo.h"

static csp_queue_handle_t qfifo[CSP_ROUTE_FIFOS];
#ifdef CSP_USE_QOS
static csp_queue_handle_t qfifo_events;
#endif

int csp_qfifo_init(void) {
	int prio;

	/* Create router fifos for each priority */
	for (prio = 0; prio < CSP_ROUTE_FIFOS; prio++) {
		if (qfifo[prio] == NULL) {
			qfifo[prio] = csp_queue_create(CSP_FIFO_INPUT, sizeof(csp_qfifo_t));
			if (!qfifo[prio])
				return CSP_ERR_NOMEM;
		}
	}

#ifdef CSP_USE_QOS
	/* Create QoS fifo notification queue */
	qfifo_events = csp_queue_create(CSP_FIFO_INPUT, sizeof(int));
	if (!qfifo_events)
		return CSP_ERR_NOMEM;
#endif

	return CSP_ERR_NONE;

}

int csp_qfifo_read(csp_qfifo_t * input) {

#ifdef CSP_USE_QOS
	int prio, found, event;

	/* Wait for packet in any queue */
	if (csp_queue_dequeue(qfifo_events, &event, FIFO_TIMEOUT) != CSP_QUEUE_OK)
		return CSP_ERR_TIMEDOUT;

	/* Find packet with highest priority */
	found = 0;
	for (prio = 0; prio < CSP_ROUTE_FIFOS; prio++) {
		if (csp_queue_dequeue(qfifo[prio], input, 0) == CSP_QUEUE_OK) {
			found = 1;
			break;
		}
	}

	if (!found) {
		csp_log_warn("Spurious wakeup: No packet found");
		return CSP_ERR_TIMEDOUT;
	}
#else
	if (csp_queue_dequeue(qfifo[0], input, FIFO_TIMEOUT) != CSP_QUEUE_OK)
		return CSP_ERR_TIMEDOUT;
#endif

	return CSP_ERR_NONE;

}

void csp_qfifo_write(csp_packet_t * packet, csp_iface_t * interface, CSP_BASE_TYPE * pxTaskWoken) {

	int result;

	if (packet == NULL) {
		csp_log_warn("csp_new packet called with NULL packet");
		return;
	} else if (interface == NULL) {
		csp_log_warn("csp_new packet called with NULL interface");
		if (pxTaskWoken == NULL)
			csp_buffer_free(packet);
		else
			csp_buffer_free_isr(packet);
		return;
	}

	csp_qfifo_t queue_element;
	queue_element.interface = interface;
	queue_element.packet = packet;

#ifdef CSP_USE_QOS
	int fifo = packet->id.pri;
#else
	int fifo = 0;
#endif

	if (pxTaskWoken == NULL)
		result = csp_queue_enqueue(qfifo[fifo], &queue_element, 0);
	else
		result = csp_queue_enqueue_isr(qfifo[fifo], &queue_element, pxTaskWoken);

#ifdef CSP_USE_QOS
	static int event = 0;

	if (result == CSP_QUEUE_OK) {
		if (pxTaskWoken == NULL)
			csp_queue_enqueue(qfifo_events, &event, 0);
		else
			csp_queue_enqueue_isr(qfifo_events, &event, pxTaskWoken);
	}
#endif

	if (result != CSP_QUEUE_OK) {
		csp_log_warn("ERROR: Routing input FIFO is FULL. Dropping packet.");
		interface->drop++;
		if (pxTaskWoken == NULL)
			csp_buffer_free(packet);
		else
			csp_buffer_free_isr(packet);
	} else {
		interface->rx++;
		interface->rxbytes += packet->length;
	}

}
