

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

#include "csp_port.h"
#include "csp_conn.h"
#include "csp_promisc.h"
#include "csp_qfifo.h"
#include "csp_rdp.h"

#if (CSP_USE_PROMISC)
extern csp_queue_handle_t csp_promisc_queue;
#endif

csp_socket_t * csp_socket(uint32_t opts) {

	/* Validate socket options */
#if (CSP_USE_RDP == 0)
	if (opts & CSP_SO_RDPREQ) {
		csp_dbg_errno = CSP_DBG_ERR_UNSUPPORTED;
		return NULL;
	}
#endif

#if (CSP_USE_HMAC == 0)
	if (opts & CSP_SO_HMACREQ) {
		csp_dbg_errno = CSP_DBG_ERR_UNSUPPORTED;
		return NULL;
	}
#endif

	/* Drop packet if reserved flags are set */
	if (opts & ~(CSP_SO_RDPREQ | CSP_SO_HMACREQ | CSP_SO_CRC32REQ | CSP_SO_CONN_LESS)) {
		csp_dbg_errno = CSP_DBG_ERR_UNSUPPORTED;
		return NULL;
	}

	/* Use CSP buffers instead? */
	csp_socket_t * sock = NULL;//csp_conn_allocate(CONN_SERVER);
	if (sock == NULL)
		return NULL;

	sock->opts = opts;

	return sock;
}

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
void csp_id_copy(csp_id_t * target, csp_id_t * source) {
	target->pri = source->pri;
	target->dst = source->dst;
	target->src = source->src;
	target->dport = source->dport;
	target->sport = source->sport;
	target->flags = source->flags;
}

int csp_send_direct(csp_id_t idout, csp_packet_t * packet, int from_me) {

	int ret;

	/* Try to find the destination on any local subnets */
	csp_iface_t * local_interface = csp_iflist_get_by_subnet(idout.dst);
	if (local_interface) {
		idout.src = local_interface->addr;
		ret = csp_send_direct_iface(idout, packet, local_interface, CSP_NO_VIA_ADDRESS, 1);

	/* Otherwise, resort to the routing table for help */		
	} else {
		csp_route_t * route = csp_rtable_find_route(idout.dst);
		if (route == NULL) {
			csp_dbg_conn_noroute++;
			return CSP_ERR_TX;
		}
		idout.src = route->iface->addr;
		ret = csp_send_direct_iface(idout, packet, route->iface, route->via, 1);
	}
	return ret;

}

__attribute__((weak)) void csp_output_hook(csp_id_t idout, csp_packet_t * packet, csp_iface_t * iface, uint16_t via, int from_me) {
	csp_print_packet("OUT: S %u, D %u, Dp %u, Sp %u, Pr %u, Fl 0x%02X, Sz %u VIA: %s (%u)\n",
				idout.src, idout.dst, idout.dport, idout.sport, idout.pri, idout.flags, packet->length, iface->name, (via != CSP_NO_VIA_ADDRESS) ? via : idout.dst);
	return;
}

int csp_send_direct_iface(csp_id_t idout, csp_packet_t * packet, csp_iface_t * iface, uint16_t via, int from_me) {

	if (iface == NULL) {
		csp_dbg_conn_noroute++;
		goto err;
	}

	csp_output_hook(idout, packet, iface, via, from_me);

	/* Copy identifier to packet (before crc and hmac) */
	csp_id_copy(&packet->id, &idout);

#if (CSP_USE_PROMISC)
	/* Loopback traffic is added to promisc queue by the router */
	if (from_me && (iface != &csp_if_lo)) {
		csp_promisc_add(packet);
	}
#endif

	/* Only encrypt packets from the current node */
	if (from_me) {

		/* Append HMAC */
		if (idout.flags & CSP_FHMAC) {
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
		if (idout.flags & CSP_FCRC32) {
			/* Calculate and add CRC32 (does not include header for backwards compatability with csp1.x) */
			if (csp_crc32_append(packet) != CSP_ERR_NONE) {
				/* CRC32 append failed */
				goto tx_err;
			}
		}

	}

	/* Store length before passing to interface */
	uint16_t bytes = packet->length;
	uint16_t mtu = iface->mtu;

	if (mtu > 0 && bytes > mtu)
		goto tx_err;

	if ((*iface->nexthop)(iface, via, packet) != CSP_ERR_NONE)
		goto tx_err;

	iface->tx++;
	iface->txbytes += bytes;
	return CSP_ERR_NONE;

tx_err:
	iface->tx_error++;
err:
	return CSP_ERR_TX;
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

	if (csp_send_direct(conn->idout, packet, 1) != CSP_ERR_NONE) {
		csp_buffer_free(packet);
		return;
	}
}

void csp_send_prio(uint8_t prio, csp_conn_t * conn, csp_packet_t * packet) {
	conn->idout.pri = prio;
	csp_send(conn, packet);
}

int csp_transaction_persistent(csp_conn_t * conn, uint32_t timeout, void * outbuf, int outlen, void * inbuf, int inlen) {

	int size = (inlen > outlen) ? inlen : outlen;
	csp_packet_t * packet = csp_buffer_get(size);
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

int csp_transaction_w_opts(uint8_t prio, uint16_t dest, uint8_t port, uint32_t timeout, void * outbuf, int outlen, void * inbuf, int inlen, uint32_t opts) {

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
	packet->id.src = 0; // The source address will be filled by csp_send_direct
	packet->id.sport = src_port;
	packet->id.pri = prio;

	if (csp_send_direct(packet->id, packet, 1) != CSP_ERR_NONE) {
		csp_buffer_free(packet);
		return;
	}
}

void csp_sendto_reply(const csp_packet_t * request_packet, csp_packet_t * reply_packet, uint32_t opts) {

	if (request_packet == NULL)
		return;

	if (opts & CSP_O_SAME) {
		reply_packet->id.flags = request_packet->id.flags;
	}

	return csp_sendto(request_packet->id.pri, request_packet->id.src, request_packet->id.sport, request_packet->id.dport, opts, reply_packet);
}
