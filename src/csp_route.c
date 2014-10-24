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
#include <csp/csp_crc32.h>
#include <csp/interfaces/csp_if_lo.h>

#include <csp/arch/csp_thread.h>
#include <csp/arch/csp_queue.h>
#include <csp/arch/csp_semaphore.h>
#include <csp/arch/csp_malloc.h>
#include <csp/arch/csp_time.h>

#include "crypto/csp_hmac.h"
#include "crypto/csp_xtea.h"

#include "csp_port.h"
#include "csp_route.h"
#include "csp_conn.h"
#include "csp_io.h"
#include "rtable/rtable.h"
#include "transport/csp_transport.h"

static csp_thread_handle_t handle_router;

static csp_queue_handle_t router_input_fifo[CSP_ROUTE_FIFOS];
#ifdef CSP_USE_QOS
static csp_queue_handle_t router_input_event;
#endif

#ifdef CSP_USE_PROMISC
csp_queue_handle_t csp_promisc_queue = NULL;
int csp_promisc_enabled = 0;
#endif

#ifdef CSP_USE_RDP
#define CSP_ROUTER_RX_TIMEOUT 100				//! If RDP is enabled, the router needs to awake some times to check timeouts
#else
#define CSP_ROUTER_RX_TIMEOUT CSP_MAX_DELAY		//! If no RDP, the router can sleep untill data arrives
#endif

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
#ifdef CSP_USE_XTEA
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
			csp_log_error("Decryption failed! Discarding packet\r\n");
			interface->autherr++;
			return CSP_ERR_XTEA;
		}
	} else if (security_opts & CSP_SO_XTEAREQ) {
		csp_log_warn("Received packet without XTEA encryption. Discarding packet\r\n");
		interface->autherr++;
		return CSP_ERR_XTEA;
#else
		csp_log_error("Received XTEA encrypted packet, but CSP was compiled without XTEA support. Discarding packet\r\n");
		interface->autherr++;
		return CSP_ERR_NOTSUP;
#endif
	}

	/* CRC32 verified packet */
	if (packet->id.flags & CSP_FCRC32) {
#ifdef CSP_USE_CRC32
		if (packet->length < 4)
			csp_log_error("Too short packet for CRC32, %u", packet->length);
		/* Verify CRC32  */
		if (csp_crc32_verify(packet) != 0) {
			/* Checksum failed */
			csp_log_error("CRC32 verification error! Discarding packet\r\n");
			interface->rx_error++;
			return CSP_ERR_CRC32;
		}
	} else if (security_opts & CSP_SO_CRC32REQ) {
		csp_log_warn("Received packet without CRC32. Accepting packet\r\n");
#else
		/* Strip CRC32 field and accept the packet */
		csp_log_warn("Received packet with CRC32, but CSP was compiled without CRC32 support. Accepting packet\r\n");
		packet->length -= sizeof(uint32_t);
#endif
	}

	/* HMAC authenticated packet */
	if (packet->id.flags & CSP_FHMAC) {
#ifdef CSP_USE_HMAC
		/* Verify HMAC */
		if (csp_hmac_verify(packet) != 0) {
			/* HMAC failed */
			csp_log_error("HMAC verification error! Discarding packet\r\n");
			interface->autherr++;
			return CSP_ERR_HMAC;
		}
	} else if (security_opts & CSP_SO_HMACREQ) {
		csp_log_warn("Received packet without HMAC. Discarding packet\r\n");
		interface->autherr++;
		return CSP_ERR_HMAC;
#else
		csp_log_error("Received packet with HMAC, but CSP was compiled without HMAC support. Discarding packet\r\n");
		interface->autherr++;
		return CSP_ERR_NOTSUP;
#endif
	}

	return CSP_ERR_NONE;

}

int csp_route_init(void) {

	int prio;

	csp_rtable_init();

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

	/* Register loopback route */
	csp_route_set(my_address, &csp_if_lo, CSP_NODE_MAC);

	/* Also register loopback as default, until user redefines default route */
	csp_route_set(CSP_DEFAULT_ROUTE, &csp_if_lo, CSP_NODE_MAC);

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

CSP_DEFINE_TASK(csp_task_router) {

	int prio;
	csp_route_queue_t input;
	csp_packet_t * packet;
	csp_conn_t * conn;
	csp_socket_t * socket;
	csp_route_t * dst;

	for (prio = 0; prio < CSP_ROUTE_FIFOS; prio++) {
		if (!router_input_fifo[prio]) {
			csp_log_error("Router %d not initialized\r\n", prio);
			csp_thread_exit();
		}
	}

	/* Here there be routing */
	while (1) {

#ifdef CSP_USE_RDP
		/* Check connection timeouts (currently only for RDP) */
		csp_conn_check_timeouts();
#endif

		/* Get next packet to route */
		if (csp_route_next_packet(&input) != CSP_ERR_NONE)
			continue;

		packet = input.packet;

		csp_log_packet("Input: Src %u, Dst %u, Dport %u, Sport %u, Pri %u, Flags 0x%02X, Size %"PRIu16"\r\n",
				packet->id.src, packet->id.dst, packet->id.dport,
				packet->id.sport, packet->id.pri, packet->id.flags, packet->length);

		/* Here there be promiscuous mode */
#ifdef CSP_USE_PROMISC
		csp_promisc_add(packet, csp_promisc_queue);
#endif

		/* If the message is not to me, route the message to the correct interface */
		if ((packet->id.dst != my_address) && (packet->id.dst != CSP_BROADCAST_ADDR)) {

			/* Find the destination interface */
			dst = csp_rtable_lookup(packet->id.dst);

			/* If the message resolves to the input interface, don't loop it back out */
			if ((dst == NULL) || ((dst->interface == input.interface) && (input.interface->split_horizon_off == 0))) {
				csp_buffer_free(packet);
				continue;
			}

			/* Otherwise, actually send the message */
			if (csp_send_direct(packet->id, packet, 0) != CSP_ERR_NONE) {
				csp_log_warn("Router failed to send\r\n");
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
			if (csp_queue_enqueue(socket->socket, &packet, 0) != CSP_QUEUE_OK) {
				csp_log_error("Conn-less socket queue full\r\n");
				csp_buffer_free(packet);
				continue;
			}
			continue;
		}

		/* Search for an existing connection */
		conn = csp_conn_find(packet->id.ext, CSP_ID_CONN_MASK);

		/* If this is an incoming packet on a new connection */
		if (conn == NULL) {

			/* Reject packet if no matching socket is found */
			if (!socket) {
				csp_buffer_free(packet);
				continue;
			}

			/* Run security check on incoming packet */
			if (csp_route_security_check(socket->opts, input.interface, packet) < 0) {
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
				csp_log_error("No more connections available\r\n");
				csp_buffer_free(packet);
				continue;
			}

			/* Store the socket queue and options */
			conn->socket = socket->socket;
			conn->opts = socket->opts;

		/* Packet to existing connection */
		} else {

			/* Run security check on incoming packet */
			if (csp_route_security_check(conn->opts, input.interface, packet) < 0) {
				csp_buffer_free(packet);
				continue;
			}

		}

		/* Pass packet to the right transport module */
		if (packet->id.flags & CSP_FRDP) {
#ifdef CSP_USE_RDP
			csp_rdp_new_packet(conn, packet);
		} else if (conn->opts & CSP_SO_RDPREQ) {
			csp_log_warn("Received packet without RDP header. Discarding packet\r\n");
			input.interface->rx_error++;
			csp_buffer_free(packet);
#else
			csp_log_error("Received RDP packet, but CSP was compiled without RDP support. Discarding packet\r\n");
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

	int ret = csp_thread_create(csp_task_router, (signed char *) "RTE", task_stack_size, NULL, priority, &handle_router);

	if (ret != 0) {
		csp_log_error("Failed to start router task\n");
		return CSP_ERR_NOMEM;
	}

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

uint8_t csp_route_get_nexthop_mac(uint8_t node) {

	csp_route_t * route = csp_rtable_lookup(node);
	return route->nexthop_mac_addr;

}

#ifdef CSP_USE_PROMISC
int csp_promisc_enable(unsigned int buf_size) {

	/* If queue already initialised */
	if (csp_promisc_queue != NULL) {
		csp_promisc_enabled = 1;
		return CSP_ERR_NONE;
	}
	
	/* Create packet queue */
	csp_promisc_queue = csp_queue_create(buf_size, sizeof(csp_packet_t *));
	
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


void csp_promisc_add(csp_packet_t * packet, csp_queue_handle_t queue) {

	if (csp_promisc_enabled == 0)
		return;

	if (queue != NULL) {
		/* Make a copy of the message and queue it to the promiscuous task */
		csp_packet_t *packet_copy = csp_buffer_clone(packet);
		if (packet_copy != NULL) {
			if (csp_queue_enqueue(queue, &packet_copy, 0) != CSP_QUEUE_OK) {
				csp_log_error("Promiscuous mode input queue full\r\n");
				csp_buffer_free(packet_copy);
			}
		}
	}

}
#endif
