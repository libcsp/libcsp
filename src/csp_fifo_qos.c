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
#include <csp/arch/csp_queue.h>
#include "csp_fifo_qos.h"

static csp_queue_handle_t router_input_fifo[CSP_ROUTE_FIFOS];
#ifdef CSP_USE_QOS
static csp_queue_handle_t router_input_event;
#endif

#ifdef CSP_USE_RDP
#define CSP_ROUTER_RX_TIMEOUT 100				//! If RDP is enabled, the router needs to awake some times to check timeouts
#else
#define CSP_ROUTER_RX_TIMEOUT CSP_MAX_DELAY		//! If no RDP, the router can sleep untill data arrives
#endif

int csp_fifo_qos_init(void) {
	int prio;

	/* Create router fifos for each priority */
	for (prio = 0; prio < CSP_ROUTE_FIFOS; prio++) {
		if (router_input_fifo[prio] == NULL) {
			router_input_fifo[prio] = csp_queue_create(CSP_FIFO_INPUT, sizeof(csp_route_queue_t));
			if (!router_input_fifo[prio])
				return CSP_ERR_NOMEM;
		}
	}

#ifdef CSP_USE_QOS
	/* Create QoS fifo notification queue */
	router_input_event = csp_queue_create(CSP_FIFO_INPUT, sizeof(int));
	if (!router_input_event)
		return CSP_ERR_NOMEM;
#endif

	return CSP_ERR_NONE;

}

int csp_route_next_packet(csp_route_queue_t * input) {

#ifdef CSP_USE_QOS
	int prio, found, event;

	/* Wait for packet in any queue */
	if (csp_queue_dequeue(router_input_event, &event, CSP_ROUTER_RX_TIMEOUT) != CSP_QUEUE_OK)
		return CSP_ERR_TIMEDOUT;

	/* Find packet with highest priority */
	found = 0;
	for (prio = 0; prio < CSP_ROUTE_FIFOS; prio++) {
		if (csp_queue_dequeue(router_input_fifo[prio], input, 0) == CSP_QUEUE_OK) {
			found = 1;
			break;
		}
	}

	if (!found) {
		csp_log_warn("Spurious wakeup of router task. No packet found\r\n");
		return CSP_ERR_TIMEDOUT;
	}
#else
	if (csp_queue_dequeue(router_input_fifo[0], input, CSP_ROUTER_RX_TIMEOUT) != CSP_QUEUE_OK)
		return CSP_ERR_TIMEDOUT;
#endif

	return CSP_ERR_NONE;

}

int csp_route_enqueue(csp_queue_handle_t handle, void * value, uint32_t timeout, CSP_BASE_TYPE * pxTaskWoken) {

	int result;

	if (pxTaskWoken == NULL)
		result = csp_queue_enqueue(handle, value, timeout);
	else
		result = csp_queue_enqueue_isr(handle, value, pxTaskWoken);

#ifdef CSP_USE_QOS
	static int event = 0;

	if (result == CSP_QUEUE_OK) {
		if (pxTaskWoken == NULL)
			csp_queue_enqueue(router_input_event, &event, 0);
		else
			csp_queue_enqueue_isr(router_input_event, &event, pxTaskWoken);
	}
#endif

	return (result == CSP_QUEUE_OK) ? CSP_ERR_NONE : CSP_ERR_NOBUFS;

}

int csp_route_get_fifo(int prio) {

#ifdef CSP_USE_QOS
	return prio;
#else
	return 0;
#endif

}

void csp_new_packet(csp_packet_t * packet, csp_iface_t * interface, CSP_BASE_TYPE * pxTaskWoken) {

	int result, fifo;

	if (packet == NULL) {
		csp_log_warn("csp_new packet called with NULL packet\r\n");
		return;
	} else if (interface == NULL) {
		csp_log_warn("csp_new packet called with NULL interface\r\n");
		if (pxTaskWoken == NULL)
			csp_buffer_free(packet);
		else
			csp_buffer_free_isr(packet);
		return;
	}

	csp_route_queue_t queue_element;
	queue_element.interface = interface;
	queue_element.packet = packet;

	fifo = csp_route_get_fifo(packet->id.pri);
	result = csp_route_enqueue(router_input_fifo[fifo], &queue_element, 0, pxTaskWoken);

	if (result != CSP_ERR_NONE) {
		csp_log_warn("ERROR: Routing input FIFO is FULL. Dropping packet.\r\n");
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
