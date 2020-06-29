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

#include "csp_qfifo.h"

#include <csp/arch/csp_queue.h>

#include "csp_init.h"

static csp_queue_handle_t qfifo[CSP_ROUTE_FIFOS];
#if (CSP_USE_QOS)
static csp_queue_handle_t qfifo_events;
#endif

int csp_qfifo_init(void) {

	/* Create router fifos for each priority */
	for (int prio = 0; prio < CSP_ROUTE_FIFOS; prio++) {
		if (qfifo[prio] == NULL) {
			qfifo[prio] = csp_queue_create(csp_conf.fifo_length, sizeof(csp_qfifo_t));
			if (!qfifo[prio])
				return CSP_ERR_NOMEM;
		}
	}

#if (CSP_USE_QOS)
	/* Create QoS fifo notification queue */
	qfifo_events = csp_queue_create(csp_conf.fifo_length, sizeof(int));
	if (!qfifo_events) {
		return CSP_ERR_NOMEM;
	}
#endif

	return CSP_ERR_NONE;

}

void csp_qfifo_free_resources(void) {

	for (int prio = 0; prio < CSP_ROUTE_FIFOS; prio++) {
		if (qfifo[prio]) {
			csp_queue_remove(qfifo[prio]);
			qfifo[prio] = NULL;
		}
	}

#if (CSP_USE_QOS)
	if (qfifo_events) {
		csp_queue_remove(qfifo_events);
		qfifo_events = NULL;
	}
#endif

}

int csp_qfifo_read(csp_qfifo_t * input) {

#if (CSP_USE_QOS)
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

void csp_qfifo_write(csp_packet_t * packet, csp_iface_t * iface, CSP_BASE_TYPE * pxTaskWoken) {

	int result;

	if (packet == NULL) {
		if (pxTaskWoken == NULL) { // Only do logging in non-ISR context
			csp_log_warn("csp_new packet called with NULL packet");
		}
		return;
	}

	if (iface == NULL) {
		if (pxTaskWoken == NULL) { // Only do logging in non-ISR context
			csp_log_warn("csp_new packet called with NULL interface");
		}
		if (pxTaskWoken == NULL)
			csp_buffer_free(packet);
		else
			csp_buffer_free_isr(packet);
		return;
	}

	csp_qfifo_t queue_element;
	queue_element.iface = iface;
	queue_element.packet = packet;

#if (CSP_USE_QOS)
	int fifo = packet->id.pri;
#else
	int fifo = 0;
#endif

	if (pxTaskWoken == NULL)
		result = csp_queue_enqueue(qfifo[fifo], &queue_element, 0);
	else
		result = csp_queue_enqueue_isr(qfifo[fifo], &queue_element, pxTaskWoken);

#if (CSP_USE_QOS)
	static int event = 0;

	if (result == CSP_QUEUE_OK) {
		if (pxTaskWoken == NULL)
			csp_queue_enqueue(qfifo_events, &event, 0);
		else
			csp_queue_enqueue_isr(qfifo_events, &event, pxTaskWoken);
	}
#endif

	if (result != CSP_QUEUE_OK) {
		if (pxTaskWoken == NULL) { // Only do logging in non-ISR context
			csp_log_warn("ERROR: Routing input FIFO is FULL. Dropping packet.");
		}
		iface->drop++;
		if (pxTaskWoken == NULL)
			csp_buffer_free(packet);
		else
			csp_buffer_free_isr(packet);
	}

}

void csp_qfifo_wake_up(void) {
	const csp_qfifo_t queue_element = {.iface = NULL, .packet = NULL};
	csp_queue_enqueue(qfifo[0], &queue_element, 0);
}
