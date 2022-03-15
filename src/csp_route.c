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

#include <stdlib.h>

#include <csp/csp_crc32.h>
#include <csp/csp_endian.h>
#include <csp/arch/csp_thread.h>
#include <csp/arch/csp_queue.h>
#include <csp/crypto/csp_hmac.h>
#include <csp/crypto/csp_xtea.h>

#include "csp_init.h"
#include "csp_port.h"
#include "csp_conn.h"
#include "csp_io.h"
#include "csp_promisc.h"
#include "csp_qfifo.h"
#include "csp_dedup.h"
#include "transport/csp_transport.h"

/**
 * Check supported packet options
 * @param iface pointer to incoming interface
 * @param packet pointer to packet
 * @return CSP_ERR_NONE is all options are supported, CSP_ERR_NOTSUP if not
 */
static int csp_route_check_options(csp_iface_t *iface, csp_packet_t *packet)
{
#if (CSP_USE_XTEA == 0)
	/* Drop XTEA packets */
	if (packet->id.flags & CSP_FXTEA) {
		csp_log_error("Received XTEA encrypted packet, but CSP was compiled without XTEA support. Discarding packet");
		iface->autherr++;
		return CSP_ERR_NOTSUP;
	}
#endif

#if (CSP_USE_HMAC == 0)
	/* Drop HMAC packets */
	if (packet->id.flags & CSP_FHMAC) {
		csp_log_error("Received packet with HMAC, but CSP was compiled without HMAC support. Discarding packet");
		iface->autherr++;
		return CSP_ERR_NOTSUP;
	}
#endif

#if (CSP_USE_RDP == 0)
	/* Drop RDP packets */
	if (packet->id.flags & CSP_FRDP) {
		csp_log_error("Received RDP packet, but CSP was compiled without RDP support. Discarding packet");
		iface->rx_error++;
		return CSP_ERR_NOTSUP;
	}
#endif
	return CSP_ERR_NONE;
}

/**
 * Helper function to decrypt, check auth and CRC32
 * @param security_opts either socket_opts or conn_opts
 * @param iface pointer to incoming interface
 * @param packet pointer to packet
 * @return #CSP_ERR_NONE on success, otherwise an error code.
 */
static int csp_route_security_check(uint32_t security_opts, csp_iface_t * iface, csp_packet_t * packet) {

#if (CSP_USE_XTEA)
	/* XTEA encrypted packet */
	if (packet->id.flags & CSP_FXTEA) {
		/* Decrypt data */
		if (csp_xtea_decrypt_packet(packet) != CSP_ERR_NONE) {
			csp_log_error("XTEA Decryption failed! Discarding packet");
			iface->autherr++;
			return CSP_ERR_XTEA;
		}
	} else if (security_opts & CSP_SO_XTEAREQ) {
		csp_log_warn("Received packet without XTEA encryption. Discarding packet");
		iface->autherr++;
		return CSP_ERR_XTEA;
	}
#endif

	/* CRC32 verified packet */
	if (packet->id.flags & CSP_FCRC32) {
#if (CSP_USE_CRC32)
		/* Verify CRC32 (does not include header for backwards compatability with csp1.x) */
		if (csp_crc32_verify(packet, false) != CSP_ERR_NONE) {
			csp_log_error("CRC32 verification error! Discarding packet");
			iface->rx_error++;
			return CSP_ERR_CRC32;
		}
#else
		/* No CRC32 validation - but size must be checked and adjusted */
		if (packet->length < sizeof(uint32_t)) {
			csp_log_error("CRC32 verification error! Discarding packet");
			iface->rx_error++;
			return CSP_ERR_CRC32;
		}
		packet->length -= sizeof(uint32_t);
#endif
	} else if (security_opts & CSP_SO_CRC32REQ) {
		csp_log_warn("Received packet with CRC32, but CSP was compiled without CRC32 support. Accepting packet");
	}

#if (CSP_USE_HMAC)
	/* HMAC authenticated packet */
	if (packet->id.flags & CSP_FHMAC) {
		/* Verify HMAC (does not include header for backwards compatability with csp1.x) */
		if (csp_hmac_verify(packet, false) != CSP_ERR_NONE) {
			/* HMAC failed */
			csp_log_error("HMAC verification error! Discarding packet");
			iface->autherr++;
			return CSP_ERR_HMAC;
		}
	} else if (security_opts & CSP_SO_HMACREQ) {
		csp_log_warn("Received packet without HMAC. Discarding packet");
		iface->autherr++;
		return CSP_ERR_HMAC;
	}
#endif

#if (CSP_USE_RDP)
	/* RDP packet */
	if (!(packet->id.flags & CSP_FRDP)) {
		if (security_opts & CSP_SO_RDPREQ) {
			csp_log_warn("Received packet without RDP header. Discarding packet");
			iface->rx_error++;
			return CSP_ERR_INVAL;
		}
	}
#endif

	return CSP_ERR_NONE;

}

int csp_route_work(uint32_t timeout) {

	csp_qfifo_t input;
	csp_packet_t * packet;
	csp_conn_t * conn;
	csp_socket_t * socket;

#if (CSP_USE_RDP)
	/* Check connection timeouts (currently only for RDP) */
	csp_conn_check_timeouts();
#endif

	/* Get next packet to route */
	if (csp_qfifo_read(&input) != CSP_ERR_NONE) {
		return CSP_ERR_TIMEDOUT;
	}

	packet = input.packet;
	if (packet == NULL) {
		return CSP_ERR_TIMEDOUT;
	}

	csp_log_packet("INP: S %u, D %u, Dp %u, Sp %u, Pr %u, Fl 0x%02X, Sz %"PRIu16" VIA: %s",
			packet->id.src, packet->id.dst, packet->id.dport,
			packet->id.sport, packet->id.pri, packet->id.flags, packet->length, input.iface->name);

	/* Here there be promiscuous mode */
#if (CSP_USE_PROMISC)
	csp_promisc_add(packet);
#endif

#if (CSP_USE_DEDUP)
	/* Check for duplicates */
	if (csp_dedup_is_duplicate(packet)) {
		/* Discard packet */
		csp_log_packet("Duplicate packet discarded");
		input.iface->drop++;
		csp_buffer_free(packet);
		return CSP_ERR_NONE;
	}
#endif

	/* Now we count the message (since its deduplicated) */
	input.iface->rx++;
	input.iface->rxbytes += packet->length;

	/* If the message is not to me, route the message to the correct interface */
	if ((packet->id.dst != csp_conf.address) && (packet->id.dst != CSP_BROADCAST_ADDR)) {

		/* Find the destination interface */
		const csp_route_t * ifroute = csp_rtable_find_route(packet->id.dst);

		/* If the message resolves to the input interface, don't loop it back out */
		if ((ifroute == NULL) || ((ifroute->iface == input.iface) && (input.iface->split_horizon_off == 0))) {
			csp_buffer_free(packet);
			return CSP_ERR_NONE;
		}

		/* Otherwise, actually send the message */
		if (csp_send_direct(packet->id, packet, ifroute, 0) != CSP_ERR_NONE) {
			csp_log_warn("Router failed to send");
			csp_buffer_free(packet);
		}

		/* Next message, please */
		return CSP_ERR_NONE;
	}

	/* Discard packets with unsupported options */
	if (csp_route_check_options(input.iface, packet) != CSP_ERR_NONE) {
		csp_buffer_free(packet);
		return CSP_ERR_NONE;
	}

	/* The message is to me, search for incoming socket */
	socket = csp_port_get_socket(packet->id.dport);

	/* If the socket is connection-less, deliver now */
	if (socket && (socket->opts & CSP_SO_CONN_LESS)) {
		if (csp_route_security_check(socket->opts, input.iface, packet) < 0) {
			csp_buffer_free(packet);
			return CSP_ERR_NONE;
		}
		if (csp_queue_enqueue(socket->socket, &packet, 0) != CSP_QUEUE_OK) {
			csp_log_error("Conn-less socket queue full");
			csp_buffer_free(packet);
			return CSP_ERR_NONE;
		}
		return CSP_ERR_NONE;
	}

	/* Search for an existing connection */
	conn = csp_conn_find(packet->id.ext, CSP_ID_CONN_MASK);

	/* If this is an incoming packet on a new connection */
	if (conn == NULL) {

		/* Reject packet if no matching socket is found */
		if (!socket) {
			csp_buffer_free(packet);
			return CSP_ERR_NONE;
		}

		/* Run security check on incoming packet */
		if (csp_route_security_check(socket->opts, input.iface, packet) < 0) {
			csp_buffer_free(packet);
			return CSP_ERR_NONE;
		}

		/* New incoming connection accepted */
		csp_id_t idout;
		idout.pri   = packet->id.pri;
		idout.src   = csp_conf.address;

		idout.dst   = packet->id.src;
		idout.dport = packet->id.sport;
		idout.sport = packet->id.dport;
		idout.flags = packet->id.flags;

		/* Create connection */
		conn = csp_conn_new(packet->id, idout);

		if (!conn) {
			csp_log_error("No more connections available");
			csp_buffer_free(packet);
			return CSP_ERR_NONE;
		}

		/* Store the socket queue and options */
		conn->socket = socket->socket;
		conn->opts = socket->opts;

	/* Packet to existing connection */
	} else {

		/* Run security check on incoming packet */
		if (csp_route_security_check(conn->opts, input.iface, packet) < 0) {
			csp_buffer_free(packet);
			return CSP_ERR_NONE;
		}

	}

#if (CSP_USE_RDP)
	/* Pass packet to RDP module */
	if (packet->id.flags & CSP_FRDP) {
		bool close_connection = csp_rdp_new_packet(conn, packet);
		if (close_connection) {
			csp_close(conn);
		}
		return CSP_ERR_NONE;
	}
#endif

	/* Pass packet to UDP module */
	csp_udp_new_packet(conn, packet);
	return CSP_ERR_NONE;
}

static CSP_DEFINE_TASK(csp_task_router) {

	/* Here there be routing */
	while (1) {
		csp_route_work(FIFO_TIMEOUT);
	}

	return CSP_TASK_RETURN;

}

int csp_route_start_task(unsigned int task_stack_size, unsigned int task_priority) {

	int ret = csp_thread_create(csp_task_router, "RTE", task_stack_size, NULL, task_priority, NULL);
	if (ret != 0) {
		csp_log_error("Failed to start router task, error: %d", ret);
		return ret;
	}

	return CSP_ERR_NONE;

}
