/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2011 GomSpace ApS (http://www.gomspace.com)
Copyright (C) 2011 AAUSAT3 Project (http://aausat3.space.aau.dk) 

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
#include <inttypes.h>

/* CSP includes */
#include <csp/csp.h>
#include <csp/csp_interface.h>
#include <csp/csp_endian.h>
#include <csp/csp_platform.h>
#include <csp/csp_error.h>

#include "arch/csp_thread.h"
#include "arch/csp_queue.h"
#include "arch/csp_semaphore.h"
#include "arch/csp_malloc.h"
#include "arch/csp_time.h"

#include "crypto/csp_hmac.h"
#include "crypto/csp_xtea.h"
#include "csp_crc32.h"

#include "csp_port.h"
#include "csp_route.h"
#include "csp_conn.h"
#include "csp_io.h"
#include "transport/csp_transport.h"

csp_thread_handle_t handle_router;

/* Static allocation of routes */
csp_iface_t * interfaces;
csp_route_t routes[CSP_ID_HOST_MAX + 2];
csp_mutex_t routes_lock;

static csp_queue_handle_t router_input_fifo[CSP_ROUTE_FIFOS];
#ifdef CSP_USE_QOS
static csp_queue_handle_t router_input_event;
#endif

#ifdef CSP_USE_PROMISC
csp_queue_handle_t csp_promisc_queue = NULL;
int csp_promisc_enabled = 0;
#endif

extern int csp_route_input_hook(csp_packet_t * packet) __attribute__((weak));

typedef struct {
	csp_iface_t * interface;
	csp_packet_t * packet;
} csp_route_queue_t;

/**
 * Helper function to decrypt, check auth and CRC32
 * @param security_opts either socket_opts or conn_opts
 * @param interface pointer to incoming interface
 * @param packet pointer to packet
 * @return -1 Missing feature, -2 XTEA error, -3 CRC error, -4 HMAC error, 0 = OK.
 */
static int csp_route_security_check(uint32_t security_opts, csp_iface_t * interface, csp_packet_t * packet) {

	/* XTEA encrypted packet */
	if (packet->id.flags & CSP_FXTEA) {
#ifdef CSP_ENABLE_XTEA
		/* Read nonce */
		uint32_t nonce;
		memcpy(&nonce, &packet->data[packet->length - sizeof(nonce)], sizeof(nonce));
		nonce = csp_ntoh32(nonce);
		packet->length -= sizeof(nonce);

		/* Create initialization vector */
		uint32_t iv[2] = {nonce, 1};

		/* Decrypt data */
		if (csp_xtea_decrypt(packet->data, packet->length, iv) != 0) {
			/* Decryption failed */
			csp_debug(CSP_ERROR, "Decryption failed! Discarding packet\r\n");
			interface->autherr++;
			return CSP_ERR_XTEA;
		}
	} else if (security_opts & CSP_SO_XTEAREQ) {
		csp_debug(CSP_WARN, "Received packet without XTEA encryption. Discarding packet\r\n");
		interface->autherr++;
		return CSP_ERR_XTEA;
#else
		csp_debug(CSP_ERROR, "Received XTEA encrypted packet, but CSP was compiled without XTEA support. Discarding packet\r\n");
		interface->autherr++;
		return CSP_ERR_NOTSUP;
#endif
	}

	/* CRC32 verified packet */
	if (packet->id.flags & CSP_FCRC32) {
#ifdef CSP_ENABLE_CRC32
		/* Verify CRC32  */
		if (csp_crc32_verify(packet) != 0) {
			/* Checksum failed */
			csp_debug(CSP_ERROR, "CRC32 verification error! Discarding packet\r\n");
			interface->rx_error++;
			return CSP_ERR_CRC32;
		}
	} else if (security_opts & CSP_SO_CRC32REQ) {
		csp_debug(CSP_WARN, "Received packet without CRC32. Discarding packet\r\n");
		interface->rx_error++;
		return CSP_ERR_CRC32;
#else
		csp_debug(CSP_ERROR, "Received packet with CRC32, but CSP was compiled without CRC32 support. Discarding packet\r\n");
		interface->rx_error++;
		return CSP_ERR_NOTSUP;
#endif
	}

	/* HMAC authenticated packet */
	if (packet->id.flags & CSP_FHMAC) {
#ifdef CSP_ENABLE_HMAC
		/* Verify HMAC */
		if (csp_hmac_verify(packet) != 0) {
			/* HMAC failed */
			csp_debug(CSP_ERROR, "HMAC verification error! Discarding packet\r\n");
			interface->autherr++;
			return CSP_ERR_HMAC;
		}
	} else if (security_opts & CSP_SO_HMACREQ) {
		csp_debug(CSP_WARN, "Received packet without HMAC. Discarding packet\r\n");
		interface->autherr++;
		return CSP_ERR_HMAC;
#else
		csp_debug(CSP_ERROR, "Received packet with HMAC, but CSP was compiled without HMAC support. Discarding packet\r\n");
		interface->autherr++;
		return CSP_ERR_NOTSUP;
#endif
	}

	return CSP_ERR_NONE;

}

int csp_route_table_init(void) {

	int prio;

	/* Clear rounting table */
	memset(routes, 0, sizeof(csp_route_t) * (CSP_ID_HOST_MAX + 2));

	/* Create routing table lock */
	if (csp_mutex_create(&routes_lock) != CSP_MUTEX_OK)
		return CSP_ERR_NOMEM;

	/* Create router fifos for each priority */
	for (prio = 0; prio < CSP_ROUTE_FIFOS; prio++) {
		router_input_fifo[prio] = csp_queue_create(CSP_FIFO_INPUT, sizeof(csp_route_queue_t));
		if (!router_input_fifo[prio])
			return CSP_ERR_NOMEM;
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
	if (csp_queue_dequeue(router_input_event, &event, 100) != CSP_QUEUE_OK)
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
		csp_debug(CSP_WARN, "Spurious wakeup of router task. No packet found\r\n");
		return CSP_ERR_TIMEDOUT;
	}
#else
	if (csp_queue_dequeue(router_input_fifo[0], input, 100) != CSP_QUEUE_OK)
		return CSP_ERR_TIMEDOUT;
#endif

	return CSP_ERR_NONE;

}
#ifndef _CSP_WINDOWS_
csp_thread_return_t vTaskCSPRouter(__attribute__ ((unused)) void * pvParameters) {
#else
csp_thread_return_t __stdcall vTaskCSPRouter(__attribute__ ((unused)) void * pvParameters) {
#endif

	int prio;
	csp_route_queue_t input;
	csp_packet_t * packet;
	csp_conn_t * conn;
	
	csp_socket_t * socket = NULL;
	csp_route_t * dst;

	for (prio = 0; prio < CSP_ROUTE_FIFOS; prio++) {
		if (!router_input_fifo[prio]) {
			csp_debug(CSP_ERROR, "Router %d not initialized\r\n", prio);
			csp_thread_exit();
		}
	}

	/* Here there be routing */
	while (1) {

		/* Check connection timeouts */
		csp_conn_check_timeouts();

		/* Get next packet to route */
		if (csp_route_next_packet(&input) != CSP_ERR_NONE)
			continue;

		/* Here is last chance to drop packet, call user hook */
		if ((csp_route_input_hook) && (csp_route_input_hook(packet) == 0)) {
			csp_buffer_free(packet);
			continue;
		}

		packet = input.packet;

		csp_debug(CSP_PACKET, "Router input: P 0x%02X, S 0x%02X, D 0x%02X, Dp 0x%02X, Sp 0x%02X, F 0x%02X\r\n",
				packet->id.pri, packet->id.src, packet->id.dst, packet->id.dport,
				packet->id.sport, packet->id.flags);

		/* Here there be promiscuous mode */
#ifdef CSP_USE_PROMISC
		csp_promisc_add(packet, csp_promisc_queue);
#endif

		/* If the message is not to me, route the message to the correct interface */
		if ((packet->id.dst != my_address) && (packet->id.dst != CSP_BROADCAST_ADDR)) {

			/* Find the destination interface */
			dst = csp_route_if(packet->id.dst);

			/* If the message resolves to the input interface, don't loop it back out */
			if ((dst == NULL) || ((dst->interface == input.interface) && (input.interface->split_horizon_off == 0))) {
				csp_buffer_free(packet);
				continue;
			}

			/* Otherwise, actually send the message */
			if (csp_send_direct(packet->id, packet, 0) != CSP_ERR_NONE) {
				csp_debug(CSP_WARN, "Router failed to send\r\n");
				csp_buffer_free(packet);
			}

			/* Next message, please */
			continue;

		}

		/* The message is to me, search for incoming socket */
		socket = csp_port_get_socket(packet->id.dport);

		/* If the socket is connection-less, deliver now */
		if (socket && (socket->opts & CSP_SO_CONN_LESS)) {
			if (csp_route_security_check(socket->opts, input.interface, packet) < 0) {
				csp_buffer_free(packet);
				continue;
			}
			if (csp_queue_enqueue(socket->queue, &packet, 0) != CSP_QUEUE_OK) {
				csp_debug(CSP_ERROR, "Conn-less socket queue full\r\n");
				csp_buffer_free(packet);
				continue;
			}
			continue;
		}

		/* Search for an existing connection */
		conn = csp_conn_find(packet->id.ext, CSP_ID_CONN_MASK);

		/* If no connection was found, try to create a new one */
		if (conn == NULL) {

			/* Reject packet if no matching socket is found */
			if (!socket) {
				csp_buffer_free(packet);
				continue;
			}

			/* New incoming connection accepted */
			csp_id_t idout;
			idout.pri   = packet->id.pri;
			idout.src   = my_address;
			idout.dst   = packet->id.src;
			idout.dport = packet->id.sport;
			idout.sport = packet->id.dport;
			idout.flags = packet->id.flags;

			/* Create connection */
			conn = csp_conn_new(packet->id, idout);

			if (!conn) {
				csp_debug(CSP_ERROR, "No more connections available\r\n");
				csp_buffer_free(packet);
				continue;
			}

			/* Store the socket queue and options */
			conn->rx_socket = socket->queue;
			conn->conn_opts = socket->opts;

		}

		/* Run security check on incoming packet */
		if (csp_route_security_check(conn->conn_opts, input.interface, packet) < 0) {
			csp_buffer_free(packet);
			continue;
		}

		/* Pass packet to the right transport module */
		if (packet->id.flags & CSP_FRDP) {
#ifdef CSP_USE_RDP
			csp_rdp_new_packet(conn, packet);
		} else if (conn->conn_opts & CSP_SO_RDPREQ) {
			csp_debug(CSP_WARN, "Received packet without RDP header. Discarding packet\r\n");
			input.interface->rx_error++;
			csp_buffer_free(packet);
#else
			csp_debug(CSP_ERROR, "Received RDP packet, but CSP was compiled without RDP support. Discarding packet\r\n");
			input.interface->rx_error++;
			csp_buffer_free(packet);
#endif
		} else {
			/* Pass packet to UDP module */
			csp_udp_new_packet(conn, packet);
		}
	}

}

int csp_route_start_task(unsigned int task_stack_size, unsigned int priority) {

	int ret = csp_thread_create(vTaskCSPRouter, (signed char *) "RTE", task_stack_size, NULL, priority, &handle_router);

	if (ret != 0) {
		csp_debug(CSP_ERROR, "Failed to start router task\n");
		return CSP_ERR_NOMEM;
	}

	return CSP_ERR_NONE;

}

int csp_route_set(uint8_t node, csp_iface_t * ifc, uint8_t nexthop_mac_addr) {

	/* Add interface to pool */
	if (ifc != NULL) {
		if (interfaces == NULL) {
			/* This is the first interface to be added */
			interfaces = ifc;
			ifc->next = NULL;
		} else {
			/* One or more interfaces were already added */
			csp_iface_t * i = interfaces;
			while (i != ifc && i->next)
				i = i->next;

			/* Insert interface last if not already in pool */
			if (i != ifc && i->next == NULL) {
				i->next = ifc;
				ifc->next = NULL;
			}
		}
	}

	/* Set route */
	if (node <= CSP_DEFAULT_ROUTE) {
		routes[node].interface = ifc;
		routes[node].nexthop_mac_addr = nexthop_mac_addr;
	} else {
		csp_debug(CSP_ERROR, "Failed to set route: invalid node id %u\r\n", node);
	}

	return CSP_ERR_NONE;

}

csp_route_t * csp_route_if(uint8_t id) {

	if (routes[id].interface != NULL) {
		return &routes[id];
	} else if (routes[CSP_DEFAULT_ROUTE].interface != NULL) {
		return &routes[CSP_DEFAULT_ROUTE];
	}
	return NULL;

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
		csp_debug(CSP_WARN, "csp_new packet called with NULL packet\r\n");
		return;
	} else if (interface == NULL) {
		csp_debug(CSP_WARN, "csp_new packet called with NULL interface\r\n");
		csp_buffer_free(packet);
		return;
	}

	csp_route_queue_t queue_element;
	queue_element.interface = interface;
	queue_element.packet = packet;

	fifo = csp_route_get_fifo(packet->id.pri);
	result = csp_route_enqueue(router_input_fifo[fifo], &queue_element, 0, pxTaskWoken);

	if (result != CSP_ERR_NONE) {
		csp_debug(CSP_WARN, "ERROR: Routing input FIFO is FULL. Dropping packet.\r\n");
		interface->drop++;
		csp_buffer_free(packet);
	} else {
		interface->rx++;
		interface->rxbytes += packet->length;
	}

}

uint8_t csp_route_get_nexthop_mac(uint8_t node) {

	csp_route_t * route = csp_route_if(node);
	return route->nexthop_mac_addr;

}

#ifdef CSP_DEBUG
static int csp_bytesize(char *buf, int len, unsigned long int n) {

	char * postfix;
	double size;

	if (n >= 1048576) {
		size = n/1048576.0;
		postfix = "M";
	} else if (n >= 1024) {
		size = n/1024.;
		postfix = "K";
	} else {
		size = n;
		postfix = "B";
	}

	return snprintf(buf, len, "%.1f%s", size, postfix);
}

void csp_route_print_interfaces(void) {

	csp_iface_t * i = interfaces;
	char txbuf[25], rxbuf[25];

	while (i) {
		csp_bytesize(txbuf, 25, i->txbytes);
		csp_bytesize(rxbuf, 25, i->rxbytes);
		printf("%-5s   tx: %05"PRIu32" rx: %05"PRIu32" txe: %05"PRIu32" rxe: %05"PRIu32"\r\n"
				"		drop: %05"PRIu32" autherr: %05"PRIu32 " frame: %05"PRIu32"\r\n"
				"		txb: %"PRIu32" (%s) rxb: %"PRIu32" (%s)\r\n\r\n",
				i->name, i->tx, i->rx, i->tx_error, i->rx_error, i->drop,
				i->autherr, i->frame, i->txbytes, txbuf, i->rxbytes, rxbuf);
		i = i->next;
	}

}

int csp_route_print_interfaces_str(char * str_buf, int str_size) {

	int printed = 0;
	csp_iface_t * i = interfaces;
	char txbuf[25], rxbuf[25];

	while (i) {
		csp_bytesize(txbuf, 25, i->txbytes);
		csp_bytesize(rxbuf, 25, i->rxbytes);
		printed += snprintf(str_buf+printed, str_size, "%-5s   tx: %05"PRIu32" rx: %05"PRIu32
				"txe: %05"PRIu32" rxe: %05"PRIu32"\r\n"
				"		drop: %05"PRIu32" autherr: %05"PRIu32 " frame: %05"PRIu32"\r\n"
				"		txb: %"PRIu32" (%s) rxb: %"PRIu32" (%s)\r\n\r\n",
				i->name, i->tx, i->rx, i->tx_error, i->rx_error, i->drop,
				i->autherr, i->frame, i->txbytes, txbuf, i->rxbytes, rxbuf);
		if ((str_size -= printed) <= 0)
			break;
		i = i->next;
	}

	return 0;

}

void csp_route_print_table(void) {

	int i;
	for (i = 0; i < CSP_DEFAULT_ROUTE; i++)
		if (routes[i].interface != NULL)
			printf("Node: %u\t\tNexthop: %s[%u]\r\n", i,
					routes[i].interface->name, routes[i].nexthop_mac_addr);
	printf("Default\t\tNexthop: %s [%u]\r\n", routes[CSP_DEFAULT_ROUTE].interface->name,
	routes[CSP_DEFAULT_ROUTE].nexthop_mac_addr);

}
#endif

#ifdef CSP_USE_PROMISC
int csp_promisc_enable(unsigned int buf_size) {

	/* If queue already initialised */
	if (csp_promisc_queue != NULL) {
		csp_promisc_enabled = 1;
		return 1;
	}
	
	/* Create packet queue */
	csp_promisc_queue = csp_queue_create(buf_size, sizeof(csp_packet_t *));
	
	if (csp_promisc_queue != NULL) {
		csp_promisc_enabled = 1;
		return 1;
	} else {
		return 0;
	}

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


void csp_promisc_add(csp_packet_t * packet, csp_queue_handle_t queue) {

	if (csp_promisc_enabled == 0)
		return;

	if (queue != NULL) {
		/* Make a copy of the message and queue it to the promiscuous task */
		csp_packet_t * packet_copy = csp_buffer_get(packet->length);
		if (packet_copy != NULL) {
			memcpy(&packet_copy->length, &packet->length, packet->length + 6);
			if (csp_queue_enqueue(queue, &packet_copy, 0) != CSP_QUEUE_OK) {
				csp_debug(CSP_ERROR, "Promiscuous mode input queue full\r\n");
				csp_buffer_free(packet_copy);
			}
		}
	}

}
#endif
