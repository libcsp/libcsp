#include "csp_io.h"

#include <stdlib.h>
#include <string.h>

#include <csp/csp.h>
#include <csp/csp_debug.h>
#include <endian.h>
#include <csp/csp_crc32.h>
#include <csp/csp_rtable.h>
#include <csp/interfaces/csp_if_lo.h>
#include <csp/arch/csp_queue.h>
#include <csp/arch/csp_time.h>
#include <csp/crypto/csp_hmac.h>
#include <csp/csp_id.h>
#include "csp_macro.h"

#include "csp_port.h"
#include "csp_conn.h"
#include "csp_promisc.h"
#include "csp_qfifo.h"
#include "csp_rdp.h"

csp_conn_t * csp_accept(csp_socket_t * sock, uint32_t timeout) {

	if ((sock == NULL) || (sock->rx_queue == NULL)) {
		csp_dbg_errno = CSP_DBG_ERR_INVALID_POINTER;
		return NULL;
	}

	csp_conn_t * conn;
	if (csp_queue_dequeue(sock->rx_queue, &conn, timeout) == CSP_QUEUE_OK) {
		return conn;
	}

	return NULL;
}

csp_packet_t * csp_read(csp_conn_t * conn, uint32_t timeout) {

	csp_packet_t * packet = NULL;

	if ((conn == NULL) || (conn->state != CONN_OPEN)) {
		return NULL;
	}

#if (CSP_USE_RDP)
	// RDP: timeout can either be 0 (for no hang poll/check) or minimum the "connection timeout"
	if (timeout && (conn->idin.flags & CSP_FRDP) && (timeout < conn->rdp.conn_timeout)) {
		timeout = conn->rdp.conn_timeout;
	}
#endif

	if (csp_queue_dequeue(conn->rx_queue, &packet, timeout) != CSP_QUEUE_OK) {
		return NULL;
	}

#if (CSP_USE_RDP)
	/* Packet read could trigger ACK transmission */
	if ((conn->idin.flags & CSP_FRDP) && conn->rdp.delayed_acks) {
		csp_rdp_check_ack(conn);
	}
#endif

	return packet;
}

/* Provide a safe method to copy type safe, between two csp ids */
void csp_id_copy(csp_id_t * target, const csp_id_t * source) {
	target->pri = source->pri;
	target->dst = source->dst;
	target->src = source->src;
	target->dport = source->dport;
	target->sport = source->sport;
	target->flags = source->flags;
}

void csp_id_clear(csp_id_t * target) {
	target->pri = 0;
	target->dst = 0;
	target->src = 0;
	target->dport = 0;
	target->sport = 0;
	target->flags = 0;
}

void csp_send_direct(csp_id_t* idout, csp_packet_t * packet, csp_iface_t * routed_from) {

	int from_me = (routed_from == NULL ? 1 : 0);

	/* Try to find the destination on any local subnets */
	int via = CSP_NO_VIA_ADDRESS;
	csp_iface_t * iface = NULL;
	csp_packet_t * copy = NULL;
	int local_found = 0;

	/* Quickly send on loopback */
	if(idout->dst == csp_if_lo.addr){
		csp_send_direct_iface(idout, packet, &csp_if_lo, via, from_me);
		return;
	}

	/* Make copy as broadcast modifies destination making iflist_get_by_subnet the skip next redundant ifaces */
	csp_id_t _idout = *idout;

	while ((iface = csp_iflist_get_by_subnet(idout->dst, iface)) != NULL) {

		local_found = 1;

		/* Do not send back to same inteface (split horizon)
		 * This check is is similar to that below, but faster */
		if (iface == routed_from) {
			continue;
		}

		/* Do not send to interface with similar subnet (split horizon) */
		if (csp_iflist_is_within_subnet(iface->addr, routed_from)) {
			continue;
		}

		/* Apply outgoing interface address to packet */
		if ((from_me) && (idout->src == 0)) {
			_idout.src = iface->addr;
		}

		/* Rewrite routed brodcast (L3) to local (L2) when arriving at the interface */
		if (csp_id_is_broadcast(idout->dst, iface)) {
			_idout.dst = csp_id_get_max_nodeid();
		}

		/* Todo: Find an elegant way to avoid making a copy when only a single destination interface
		 * is found. But without looping the list twice. And without using stack memory.
		 * Is this even possible? */
		copy = csp_buffer_clone(packet);
		if (copy != NULL) {
			csp_send_direct_iface(&_idout, copy, iface, via, from_me);
		}

	}

	/* If the above worked, we don't want to look at the routing table */
	if (local_found == 1) {
		csp_buffer_free(packet);
		return;
	}

#if CSP_USE_RTABLE
	/* Try to send via routing table */
	int route_found = 0;
	csp_route_t * route = csp_rtable_find_route(idout->dst);
	if (route != NULL) {
		do {
			route_found = 1;

			/* Do not send back to same inteface (split horizon)
			* This check is is similar to that below, but faster */
			if (route->iface == routed_from) {
				continue;
			}

			/* Do not send to interface with similar subnet (split horizon) */
			if (csp_iflist_is_within_subnet(route->iface->addr, routed_from)) {
				continue;
			}

			/* Apply outgoing interface address to packet */
			if ((from_me) && (idout->src == 0)) {
				idout->src = route->iface->addr;
			}

			copy = csp_buffer_clone(packet);
			if (copy != NULL) {
				csp_send_direct_iface(idout, copy, route->iface, route->via, from_me);
			}
		} while ((route = csp_rtable_search_backward(route)) != NULL);
	}

	/* If the above worked, we don't want to look at default interfaces */
	if (route_found == 1) {
		csp_buffer_free(packet);
		return;
	}

#endif

	/* Try to send via default interfaces */
	while ((iface = csp_iflist_get_by_isdfl(iface)) != NULL) {

		/* Do not send back to same inteface (split horizon)
		 * This check is is similar to that below, but faster */
		if (iface == routed_from) {
			continue;
		}

		/* Do not send to interface with similar subnet (split horizon) */
		if (csp_iflist_is_within_subnet(iface->addr, routed_from)) {
			continue;
		}

		/* Apply outgoing interface address to packet */
		if ((from_me) && (idout->src == 0)) {
			idout->src = iface->addr;
		}

		/* Todo: Find an elegant way to avoid making a copy when only a single destination interface
		 * is found. But without looping the list twice. And without using stack memory.
		 * Is this even possible? */
		copy = csp_buffer_clone(packet);
		if (copy != NULL) {
			csp_send_direct_iface(idout, copy, iface, via, from_me);
		}

	}

	csp_buffer_free(packet);

}

__weak void csp_output_hook(const csp_id_t * idout, csp_packet_t * packet, csp_iface_t * iface, uint16_t via, int from_me) {
	csp_print_packet("OUT: S %u, D %u, Dp %u, Sp %u, Pr %u, Fl 0x%02X, Sz %u VIA: %s (%u), Tms %u\n",
				idout->src, idout->dst, idout->dport, idout->sport, idout->pri, idout->flags, packet->length, iface->name, (via != CSP_NO_VIA_ADDRESS) ? via : idout->dst, csp_get_ms());
	return;
}

void csp_send_direct_iface(const csp_id_t* idout, csp_packet_t * packet, csp_iface_t * iface, uint16_t via, int from_me) {

	csp_output_hook(idout, packet, iface, via, from_me);

	/* Copy identifier to packet (before crc and hmac) */
	if(idout != &packet->id) {
		csp_id_copy(&packet->id, idout);
	}


	/* Only encrypt packets from the current node */
	if (from_me) {
		/* Append HMAC */
		if (idout->flags & CSP_FHMAC) {
#if (CSP_USE_HMAC)
			/* Calculate and add HMAC (does not include header for backwards compatability with csp1.x) */
			if (csp_hmac_append(packet, false) != CSP_ERR_NONE) {
				/* HMAC append failed */
				goto tx_err;
			}
#else
			csp_dbg_errno = CSP_DBG_ERR_UNSUPPORTED;
			goto tx_err;
#endif
		}

		/* Append CRC32 */
		if (idout->flags & CSP_FCRC32) {
			/* Calculate and add CRC32 (does not include header for backwards compatability with csp1.x) */
			if (csp_crc32_append(packet) != CSP_ERR_NONE) {
				/* CRC32 append failed */
				goto tx_err;
			}
		}

#if (CSP_USE_PROMISC)
		/* Loopback traffic is added to promisc queue by the router */
		if (iface != &csp_if_lo) {
			csp_promisc_add(packet);
		}
#endif
	}

	/* Store length before passing to interface */
	uint16_t bytes = packet->length;

	if ((*iface->nexthop)(iface, via, packet, from_me) != CSP_ERR_NONE)
		goto tx_err;

	iface->tx++;
	iface->txbytes += bytes;

	return;

tx_err:
	csp_buffer_free(packet);
	iface->tx_error++;
	return;
}

void csp_send(csp_conn_t * conn, csp_packet_t * packet) {

	if (packet == NULL) {
		return;
	}

	if ((conn == NULL) || (conn->state != CONN_OPEN)) {
		csp_buffer_free(packet);
		return;
	}

#if (CSP_USE_RDP)
	if (conn->idout.flags & CSP_FRDP) {
		if (csp_rdp_send(conn, packet) != CSP_ERR_NONE) {
			csp_buffer_free(packet);
			return;
		}
	}
#endif

	csp_send_direct(&conn->idout, packet, NULL);

}

void csp_send_prio(uint8_t prio, csp_conn_t * conn, csp_packet_t * packet) {
	conn->idout.pri = prio;
	csp_send(conn, packet);
}

int csp_transaction_persistent(csp_conn_t * conn, uint32_t timeout, const void * outbuf, int outlen, void * inbuf, int inlen) {

	if(outlen > CSP_BUFFER_SIZE){
		return 0;
	}

	csp_packet_t * packet = csp_buffer_get(0);
	if (packet == NULL)
		return 0;

	/* Copy the request */
	if (outlen > 0 && outbuf != NULL)
		memcpy(packet->data, outbuf, outlen);
	packet->length = outlen;

	csp_send(conn, packet);

	/* If no reply is expected, return now */
	if (inlen == 0)
		return 1;

	packet = csp_read(conn, timeout);
	if (packet == NULL)
		return 0;

	if ((inlen != -1) && ((int)packet->length != inlen)) {
		csp_dbg_inval_reply++;
		csp_buffer_free(packet);
		return 0;
	}

	memcpy(inbuf, packet->data, packet->length);
	int length = packet->length;
	csp_buffer_free(packet);
	return length;
}

int csp_transaction_w_opts(uint8_t prio, uint16_t dest, uint8_t port, uint32_t timeout, const void * outbuf, int outlen, void * inbuf, int inlen, uint32_t opts) {

	csp_conn_t * conn = csp_connect(prio, dest, port, 0, opts);
	if (conn == NULL)
		return 0;

	int status = csp_transaction_persistent(conn, timeout, outbuf, outlen, inbuf, inlen);

	csp_close(conn);

	return status;
}

csp_packet_t * csp_recvfrom(csp_socket_t * socket, uint32_t timeout) {

	if ((socket == NULL) || (!(socket->opts & CSP_SO_CONN_LESS)))
		return NULL;

	csp_packet_t * packet = NULL;
	csp_queue_dequeue(socket->rx_queue, &packet, timeout);

	return packet;
}

void csp_sendto(uint8_t prio, uint16_t dest, uint8_t dport, uint8_t src_port, uint32_t opts, csp_packet_t * packet) {

	if (!(opts & CSP_O_SAME))
		packet->id.flags = 0;

	if (opts & CSP_O_RDP) {
		csp_dbg_errno = CSP_DBG_ERR_UNSUPPORTED;
		csp_buffer_free(packet);
		return;
	}

	if (opts & CSP_O_HMAC) {
#if (CSP_USE_HMAC)
		packet->id.flags |= CSP_FHMAC;
#else
		csp_dbg_errno = CSP_DBG_ERR_UNSUPPORTED;
		csp_buffer_free(packet);
		return;
#endif
	}

	if (opts & CSP_O_CRC32) {
		packet->id.flags |= CSP_FCRC32;
	}

	packet->id.dst = dest;
	packet->id.dport = dport;
	packet->id.sport = src_port;
	packet->id.pri = prio;

	csp_send_direct(&packet->id, packet, NULL);

}

void csp_sendto_reply(const csp_packet_t * request_packet, csp_packet_t * reply_packet, uint32_t opts) {

	if (request_packet == NULL)
		return;

	if (opts & CSP_O_SAME) {
		reply_packet->id.flags = request_packet->id.flags;
	}
	uint16_t dst = request_packet->id.src;
	if (request_packet->id.dst != csp_id_get_max_nodeid()) {
		reply_packet->id.src = request_packet->id.dst;
	} else {
		reply_packet->id.src = 0;
	}
	csp_sendto(request_packet->id.pri, dst, request_packet->id.sport, request_packet->id.dport, opts, reply_packet);
}
