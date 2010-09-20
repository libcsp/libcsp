/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2010 GomSpace ApS (gomspace.com)
Copyright (C) 2010 AAUSAT3 Project (aausat3.space.aau.dk) 

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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/* CSP includes */
#include <csp/csp.h>
#include <csp/csp_platform.h>

#include "arch/csp_thread.h"
#include "arch/csp_queue.h"
#include "arch/csp_semaphore.h"
#include "arch/csp_malloc.h"
#include "arch/csp_time.h"

#include "csp_port.h"
#include "csp_route.h"
#include "csp_conn.h"
#include "csp_io.h"
#include "transport/csp_transport.h"

/* Static allocation of interfaces */
csp_iface_t iface[CSP_ID_HOST_MAX + 2];

#if CSP_USE_PROMISC
csp_queue_handle_t csp_promisc_queue = NULL;
#endif

csp_thread_handle_t handle_router;

extern int csp_route_input_hook(void) __attribute__((weak));

/** Routing input Queue
 * This queue is used each time a packet is received from an IF.
 * It holds the csp_route_queue_t complex datatype
 */
static csp_queue_handle_t router_input_fifo = NULL;
typedef struct csp_route_queue_s {
	void * interface;
	csp_packet_t * packet;
} csp_route_queue_t;

/** csp_route_table_init
 * Initialises the storage for the routing table
 */
void csp_route_table_init(void) {

	/* Clear table */
	memset(iface, 0, sizeof(csp_iface_t) * (CSP_ID_HOST_MAX + 2));

}

/** Router Task
 * This task received any non-local connection and collects the data
 * on the connection. All data is forwarded out of the router
 * using the csp_send call 
 */
csp_thread_return_t vTaskCSPRouter(void * pvParameters) {

	csp_route_queue_t input;
	csp_packet_t * packet;
	csp_conn_t * conn;
	
	csp_queue_handle_t queue = NULL;
	csp_iface_t * dst;

	/* Create fallback socket  */
	router_input_fifo = csp_queue_create(CSP_FIFO_INPUT, sizeof(csp_route_queue_t));

    /* Here there be routing */
	while (1) {

		/* Check connection timeouts */
		csp_conn_check_timeouts();

		/* Receive input */
		if (csp_queue_dequeue(router_input_fifo, &input, 100) != CSP_QUEUE_OK)
			continue;

		/* Discard invalid */
		if (input.packet == NULL) {
			csp_debug(CSP_ERROR, "Invalid packet in router queue\r\n");
			continue;
		}

		packet = input.packet;

		/* Here is last chance to drop packet, call user hook */
		if ((csp_route_input_hook) && (csp_route_input_hook() == 0)) {
			csp_buffer_free(packet);
			continue;
		}

		csp_debug(CSP_PACKET, "Router input: P 0x%02X, S 0x%02X, D 0x%02X, Dp 0x%02X, Sp 0x%02X\r\n",
				packet->id.pri, packet->id.src, packet->id.dst, packet->id.dport,
				packet->id.sport);

		/* Here there be promiscous mode */
#if CSP_USE_PROMISC
		csp_promisc_add(packet, csp_promisc_queue);
#endif

		/* If the message is not to me, route the message to the correct iface */
		if ((packet->id.dst != my_address) && (packet->id.dst != CSP_BROADCAST_ADDR)) {

			/* Find the destination interface */
			dst = csp_route_if(packet->id.dst);

			/* If the message resolves to the input interface, don't loop ip back out */
			if ((dst == NULL) || (dst->nexthop == input.interface)) {
				csp_buffer_free(packet);
				continue;
			}

			/* Otherwise, actually send the message */
			if (!csp_send_direct(packet->id, packet, 0))
			   csp_buffer_free(packet);

            /* Next message, please */
			continue;

		}

		/* Now, the message is to me:
		 * search for an existing connection */
		conn = csp_conn_find(packet->id.ext, CSP_ID_CONN_MASK);

		/* If a connection was found */
		if (conn != NULL) {

			/* Check the close_wait state */
			if (conn->state == CONN_CLOSE_WAIT) {
				csp_debug(CSP_WARN, "Router discarded packet: CLOSE_WAIT\r\n");
				csp_buffer_free(packet);
				continue;
			}

		/* Okay, this is a new connection attempt,
		 * check if a port is listening and open a conn.
		 */
		} else {
            
            /* Reject packet if dport is an ephemeral port */
            if (packet->id.dport > CSP_MAX_BIND_PORT + 1) {
		        csp_buffer_free(packet); 
		    	continue;
	    	}
    
			/* Try to deliver to incoming port number */
			if (ports[packet->id.dport].state == PORT_OPEN) {
				queue = ports[packet->id.dport].socket->conn_queue;

			/* Otherwise, try local "catch all" port number */
			} else if (ports[CSP_ANY].state == PORT_OPEN) {
				queue = ports[CSP_ANY].socket->conn_queue;

			/* Or reject */
			} else {
				csp_buffer_free(packet);
				continue;
			}

			/* New incoming connection accepted */
			csp_id_t idout;
			idout.pri = packet->id.pri;
			idout.dst = packet->id.src;
			idout.dport = packet->id.sport;
			idout.sport = packet->id.dport;
			idout.protocol = packet->id.protocol;

			/* Ensure a broadcast packet is replied from correct source address */
			if (packet->id.dst == CSP_BROADCAST_ADDR) {
				idout.src = my_address;
			} else {
				idout.src = packet->id.dst;
			}

			conn = csp_conn_new(packet->id, idout);

			if (conn == NULL) {
				csp_debug(CSP_ERROR, "No more connections available\r\n");
				csp_buffer_free(packet);
				continue;
			}

			/* Store the queue to be posted to */
			conn->rx_socket = queue;

		}

		/* Pass packet to the right transport module */
		switch(packet->id.protocol) {
#if CSP_USE_RDP
		case CSP_RDP:
			csp_rdp_new_packet(conn, packet);
			break;
#endif
		case CSP_UDP:
			csp_udp_new_packet(conn, packet);
			break;
		default:
			csp_buffer_free(packet);
			break;
		}


	}

}

/**
 * Use this function to start the router task.
 * @param task_stack_size The number of portStackType to allocate. This only affects FreeRTOS systems.
 */

void csp_route_start_task(unsigned int task_stack_size, unsigned int priority) {

	int ret = csp_thread_create(vTaskCSPRouter, (signed char *) "RTE", task_stack_size, NULL, priority, &handle_router);

	if (ret != 0)
		csp_debug(CSP_ERROR, "Failed to start router task\n");

}

/** Set route
 * This function maintains the routing table,
 * To set default route use nodeid CSP_DEFAULT_ROUTE
 * To set a value pass a callback function
 * To clear a value pass a NULL value
 */
void csp_route_set(const char * name, uint8_t node, nexthop_t nexthop, uint8_t nexthop_mac_addr) {

	if (node <= CSP_DEFAULT_ROUTE) {
		iface[node].nexthop = nexthop;
		iface[node].name = name;
		iface[node].nexthop_mac_addr = nexthop_mac_addr;
	} else {
		csp_debug(CSP_ERROR, "Failed to set route: invalid nodeid %u\r\n", node);
	}

}

/** Routing table lookup
 * This is the actual lookup in the routing table
 * The table consists of one entry per possible node
 * If there is no explicit nexthop route for the destination
 * the default route (node CSP_DEFAULT_ROUTE) is used.
 */
csp_iface_t * csp_route_if(uint8_t id) {

	if (iface[id].nexthop != NULL) {
		return &iface[id];
	}
	if (iface[CSP_DEFAULT_ROUTE].nexthop != NULL) {
		return &iface[CSP_DEFAULT_ROUTE];
	}
	return NULL;

}

/**
 * Inputs a new packet into the system
 * This function is called from interface drivers ISR to route and accept packets.
 * But it can also be called from a task, provided that the pxTaskWoken parameter is NULL!
 *
 * EXTREMELY IMPORTANT:
 * pxTaskWoken arg must ALWAYS be NULL if called from task,
 * and ALWAYS be NON NULL if called from ISR!
 * If this condition is met, this call is completely thread-safe
 *
 * This function is fire and forget, it returns void, meaning
 * that a packet will always be either accepted or dropped
 * so the memory will always be freed.
 *
 * @param packet A pointer to the incoming packet
 * @param interface A pointer to the incoming interface TX function.
 * @param pxTaskWoken This must be a pointer a valid variable if called from ISR or NULL otherwise!
 *
 */
void csp_new_packet(csp_packet_t * packet, nexthop_t interface, CSP_BASE_TYPE * pxTaskWoken) {

	int result;

	if (router_input_fifo == NULL) {
	    csp_debug(CSP_WARN, "WARNING: csp_new_packet called with NULL router_input_fifo\r\n");
	    csp_buffer_free(packet);
		return;
	}

	csp_route_queue_t queue_element;
	queue_element.interface = interface;
	queue_element.packet = packet;

	if (pxTaskWoken == NULL) {
		result = csp_queue_enqueue(router_input_fifo, &queue_element, 0);
	} else {
		result = csp_queue_enqueue_isr(router_input_fifo, &queue_element, pxTaskWoken);
	}

	if (result != CSP_QUEUE_OK) {
		csp_debug(CSP_WARN, "ERROR: Routing input FIFO is FULL. Dropping packet.\r\n");
		csp_buffer_free(packet);
	}

}

uint8_t csp_route_get_nexthop_mac(uint8_t node) {
	csp_iface_t * iface = csp_route_if(node);
	return iface->nexthop_mac_addr;
}

#if CSP_DEBUG
void csp_route_print_table(void) {

	int i;
	for (i = 0; i < CSP_DEFAULT_ROUTE; i++)
		if (iface[i].nexthop != NULL)
			printf("\tNode: %u\t\tNexthop: %s[%u]\t\tCount: %u\r\n", i,
					iface[i].name, iface[i].nexthop_mac_addr, iface[i].count);
	printf("\tDefault\t\tNexthop: %s [%u]\t\tCount: %u\r\n", iface[CSP_DEFAULT_ROUTE].name, iface[CSP_DEFAULT_ROUTE].nexthop_mac_addr, iface[CSP_DEFAULT_ROUTE].count);

}
#endif

#if CSP_USE_PROMISC
/**
 * Enable promiscuous mode packet queue
 * This function is used to enable promiscuous mode for the router.
 * If enabled, a copy of all incoming packets are placed in a queue
 * that can be read with csp_promisc_get().
 *
 * Not all interface drivers support promiscuous mode. 
 *
 * @param buf_size Size of buffer for incoming packets
 *
 */
int csp_promisc_enable(unsigned int buf_size) {
    if (csp_promisc_queue != NULL)
        return 0;
    
    /* Create packet queue */
    csp_promisc_queue = csp_queue_create(buf_size, sizeof(csp_packet_t *));
    
    if (csp_promisc_queue != NULL) {
	return 1;
    } else {
	return 0;
    }
}

/**
 * Get packet from promiscuous mode packet queue
 * Returns the first packet from the promiscuous mode packet queue.
 * The queue is FIFO, so the returned packet is the oldest one
 * in the queue. 
 *
 * @param timeout Timeout in ms to wait for a new packet
 *
 */
csp_packet_t * csp_promisc_read(unsigned int timeout) {

    if (csp_promisc_queue == NULL)
	return NULL;

    csp_packet_t * packet = NULL;
    csp_queue_dequeue(csp_promisc_queue, &packet, timeout);

    return packet;

}

/**
 * Add packet to promiscuous mode packet queue 
 *
 * @param packet Packet to add to the queue
 * @param queue Promiscuous mode packet queue
 *
 */
void csp_promisc_add(csp_packet_t * packet, csp_queue_handle_t queue) {

	if (queue != NULL) {

		/* Make a copy of the message and queue it to the promisc task */
		csp_packet_t * packet_copy = csp_buffer_get(packet->length);
		if (packet_copy != NULL) {
			memcpy(&packet_copy->length, &packet->length, packet->length + 6);
			if (csp_queue_enqueue(queue, &packet_copy, 0) != CSP_QUEUE_OK) {
				csp_debug(CSP_ERROR, "Promisc. mode input queue full\r\n");
				csp_buffer_free(packet_copy);
			}
		}

	}

}
#endif
