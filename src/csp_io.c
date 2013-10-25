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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* CSP includes */
#include <csp/csp.h>
#include <csp/csp_error.h>
#include <csp/csp_endian.h>
#include <csp/csp_crc32.h>
#include <csp/interfaces/csp_if_lo.h>

#include <csp/arch/csp_thread.h>
#include <csp/arch/csp_queue.h>
#include <csp/arch/csp_semaphore.h>
#include <csp/arch/csp_time.h>
#include <csp/arch/csp_malloc.h>

#include "crypto/csp_hmac.h"
#include "crypto/csp_xtea.h"

#include "csp_io.h"
#include "csp_port.h"
#include "csp_conn.h"
#include "csp_route.h"
#include "transport/csp_transport.h"

/** Static local variables */
unsigned char my_address;

/* Hostname and model */ 
static char * csp_hostname = NULL;
static char * csp_model = NULL;

#ifdef CSP_USE_PROMISC
extern csp_queue_handle_t csp_promisc_queue;
#endif

void csp_set_hostname(char * hostname) {
	csp_hostname = hostname;
}

char * csp_get_hostname(void) {
	return csp_hostname;
}

void csp_set_model(char * model) {
	csp_model = model;
}

char * csp_get_model(void) {
	return csp_model;
}

int csp_init(unsigned char address) {

	int ret;

	/* Initialize CSP */
	my_address = address;

	ret = csp_conn_init();
	if (ret != CSP_ERR_NONE)
		return ret;

	ret = csp_port_init();
	if (ret != CSP_ERR_NONE)
		return ret;

	ret = csp_route_table_init();
	if (ret != CSP_ERR_NONE)
		return ret;

	return CSP_ERR_NONE;

}

csp_socket_t * csp_socket(uint32_t opts) {
	
	/* Validate socket options */
#ifndef CSP_USE_RDP
	if (opts & CSP_SO_RDPREQ) {
		csp_log_error("Attempt to create socket that requires RDP, but CSP was compiled without RDP support\r\n");
		return NULL;
	}
#endif

#ifndef CSP_USE_XTEA
	if (opts & CSP_SO_XTEAREQ) {
		csp_log_error("Attempt to create socket that requires XTEA, but CSP was compiled without XTEA support\r\n");
		return NULL;
	}
#endif

#ifndef CSP_USE_HMAC
	if (opts & CSP_SO_HMACREQ) {
		csp_log_error("Attempt to create socket that requires HMAC, but CSP was compiled without HMAC support\r\n");
		return NULL;
	} 
#endif

#ifndef CSP_USE_CRC32
	if (opts & CSP_SO_CRC32REQ) {
		csp_log_error("Attempt to create socket that requires CRC32, but CSP was compiled without CRC32 support\r\n");
		return NULL;
	} 
#endif
	
	/* Drop packet if reserved flags are set */
	if (opts & ~(CSP_SO_RDPREQ | CSP_SO_XTEAREQ | CSP_SO_HMACREQ | CSP_SO_CRC32REQ | CSP_SO_CONN_LESS)) {
		csp_log_error("Invalid socket option\r\n");
		return NULL;
	}

	/* Use CSP buffers instead? */
	csp_socket_t * sock = csp_conn_allocate(CONN_SERVER);
	if (sock == NULL)
		return NULL;

	/* If connectionless, init the queue to a pre-defined size
	 * if not, the user must init the queue using csp_listen */
	if (opts & CSP_SO_CONN_LESS) {
		sock->socket = csp_queue_create(CSP_CONN_QUEUE_LENGTH, sizeof(csp_packet_t *));
		if (sock->socket == NULL)
			return NULL;
	} else {
		sock->socket = NULL;
	}
	sock->opts = opts;

	return sock;

}

csp_conn_t * csp_accept(csp_socket_t * sock, uint32_t timeout) {

	if (sock == NULL)
		return NULL;

	if (sock->socket == NULL)
		return NULL;

	csp_conn_t * conn;
	if (csp_queue_dequeue(sock->socket, &conn, timeout) == CSP_QUEUE_OK)
		return conn;

	return NULL;

}

csp_packet_t * csp_read(csp_conn_t * conn, uint32_t timeout) {

	csp_packet_t * packet = NULL;

	if (conn == NULL || conn->state != CONN_OPEN)
		return NULL;

#ifdef CSP_USE_QOS
	int prio, event;
	if (csp_queue_dequeue(conn->rx_event, &event, timeout) != CSP_QUEUE_OK)
		return NULL;

	for (prio = 0; prio < CSP_RX_QUEUES; prio++)
		if (csp_queue_dequeue(conn->rx_queue[prio], &packet, 0) == CSP_QUEUE_OK)
			break;
#else
	if (csp_queue_dequeue(conn->rx_queue[0], &packet, timeout) != CSP_QUEUE_OK)
		return NULL;
#endif

#ifdef CSP_USE_RDP
	/* Packet read could trigger ACK transmission */
	if (conn->idin.flags & CSP_FRDP)
		csp_rdp_check_ack(conn);
#endif

	return packet;

}

int csp_send_direct(csp_id_t idout, csp_packet_t * packet, uint32_t timeout) {

	if (packet == NULL) {
		csp_log_error("csp_send_direct called with NULL packet\r\n");
		goto err;
	}

	csp_route_t * ifout = csp_route_if(idout.dst);

	if ((ifout == NULL) || (ifout->interface == NULL) || (ifout->interface->nexthop == NULL)) {
		csp_log_error("No route to host: %#08x\r\n", idout.ext);
		goto err;
	}

	csp_log_packet("Output: Src %u, Dst %u, Dport %u, Sport %u, Pri %u, Flags 0x%02X, Size %u VIA: %s\r\n",
		idout.src, idout.dst, idout.dport, idout.sport, idout.pri, idout.flags, packet->length, ifout->interface->name);

#ifdef CSP_USE_PROMISC
	/* Loopback traffic is added to promisc queue by the router */
	if (idout.dst != my_address && idout.src == my_address) {
		packet->id.ext = idout.ext;
		csp_promisc_add(packet, csp_promisc_queue);
	}
#endif

	/* Only encrypt packets from the current node */
	if (idout.src == my_address) {
		/* Append HMAC */
		if (idout.flags & CSP_FHMAC) {
#ifdef CSP_USE_HMAC
			/* Calculate and add HMAC */
			if (csp_hmac_append(packet) != 0) {
				/* HMAC append failed */
				csp_log_warn("HMAC append failed!\r\n");
				goto tx_err;
			}
#else
			csp_log_warn("Attempt to send packet with HMAC, but CSP was compiled without HMAC support. Discarding packet\r\n");
			goto tx_err;
#endif
		}

		/* Append CRC32 */
		if (idout.flags & CSP_FCRC32) {
#ifdef CSP_USE_CRC32
			/* Calculate and add CRC32 */
			if (csp_crc32_append(packet) != 0) {
				/* CRC32 append failed */
				csp_log_warn("CRC32 append failed!\r\n");
				goto tx_err;
			}
#else
			csp_log_warn("Attempt to send packet with CRC32, but CSP was compiled without CRC32 support. Sending without CRC32r\n");
			idout.flags &= ~(CSP_FCRC32);
#endif
		}

		if (idout.flags & CSP_FXTEA) {
#ifdef CSP_USE_XTEA
			/* Create nonce */
			uint32_t nonce, nonce_n;
			nonce = (uint32_t)rand();
			nonce_n = csp_hton32(nonce);
			memcpy(&packet->data[packet->length], &nonce_n, sizeof(nonce_n));

			/* Create initialization vector */
			uint32_t iv[2] = {nonce, 1};

			/* Encrypt data */
			if (csp_xtea_encrypt(packet->data, packet->length, iv) != 0) {
				/* Encryption failed */
				csp_log_warn("Encryption failed! Discarding packet\r\n");
				goto tx_err;
			}

			packet->length += sizeof(nonce_n);
#else
			csp_log_warn("Attempt to send XTEA encrypted packet, but CSP was compiled without XTEA support. Discarding packet\r\n");
			goto tx_err;
#endif
		}
	}

	/* Copy identifier to packet */
	packet->id.ext = idout.ext;

	/* Store length before passing to interface */
	uint16_t bytes = packet->length;
	uint16_t mtu = ifout->interface->mtu;

	if (mtu > 0 && bytes > mtu)
		goto tx_err;

	if ((*ifout->interface->nexthop)(ifout->interface, packet, timeout) != CSP_ERR_NONE)
		goto tx_err;

	ifout->interface->tx++;
	ifout->interface->txbytes += bytes;
	return CSP_ERR_NONE;

tx_err:
	ifout->interface->tx_error++;
err:
	return CSP_ERR_TX;

}

int csp_send(csp_conn_t * conn, csp_packet_t * packet, uint32_t timeout) {

	int ret;

	if ((conn == NULL) || (packet == NULL) || (conn->state != CONN_OPEN)) {
		csp_log_error("Invalid call to csp_send\r\n");
		return 0;
	}

#ifdef CSP_USE_RDP
	if (conn->idout.flags & CSP_FRDP) {
		if (csp_rdp_send(conn, packet, timeout) != CSP_ERR_NONE) {
			csp_route_t * ifout = csp_route_if(conn->idout.dst);
			if (ifout != NULL && ifout->interface != NULL)
				ifout->interface->tx_error++;
			csp_log_warn("RDP send failed\r\n!");
			return 0;
		}
	}
#endif

	ret = csp_send_direct(conn->idout, packet, timeout);

	return (ret == CSP_ERR_NONE) ? 1 : 0;

}

int csp_send_prio(uint8_t prio, csp_conn_t * conn, csp_packet_t * packet, uint32_t timeout) {
	conn->idout.pri = prio;
	return csp_send(conn, packet, timeout);
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

	if (!csp_send(conn, packet, timeout)) {
		csp_buffer_free(packet);
		return 0;
	}

	/* If no reply is expected, return now */
	if (inlen == 0)
		return 1;

	packet = csp_read(conn, timeout);
	if (packet == NULL)
		return 0;

	if ((inlen != -1) && ((int)packet->length != inlen)) {
		csp_log_error("Reply length %u expected %u\r\n", packet->length, inlen);
		csp_buffer_free(packet);
		return 0;
	}

	memcpy(inbuf, packet->data, packet->length);
	int length = packet->length;
	csp_buffer_free(packet);
	return length;

}

int csp_transaction(uint8_t prio, uint8_t dest, uint8_t port, uint32_t timeout, void * outbuf, int outlen, void * inbuf, int inlen) {

	csp_conn_t * conn = csp_connect(prio, dest, port, 0, CSP_O_NONE);
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
	csp_queue_dequeue(socket->socket, &packet, timeout);

	return packet;

}

int csp_sendto(uint8_t prio, uint8_t dest, uint8_t dport, uint8_t src_port, uint32_t opts, csp_packet_t * packet, uint32_t timeout) {

	packet->id.flags = 0;

	if (opts & CSP_O_RDP) {
		csp_log_error("Attempt to create RDP packet on connection-less socket\r\n");
		return CSP_ERR_INVAL;
	}

	if (opts & CSP_O_HMAC) {
#ifdef CSP_USE_HMAC
		packet->id.flags |= CSP_FHMAC;
#else
		csp_log_error("Attempt to create HMAC authenticated packet, but CSP was compiled without HMAC support\r\n");
		return CSP_ERR_NOTSUP;
#endif
	}

	if (opts & CSP_O_XTEA) {
#ifdef CSP_USE_XTEA
		packet->id.flags |= CSP_FXTEA;
#else
		csp_log_error("Attempt to create XTEA encrypted packet, but CSP was compiled without XTEA support\r\n");
		return CSP_ERR_NOTSUP;
#endif
	}

	if (opts & CSP_O_CRC32) {
#ifdef CSP_USE_CRC32
		packet->id.flags |= CSP_FCRC32;
#else
		csp_log_error("Attempt to create CRC32 validated packet, but CSP was compiled without CRC32 support\r\n");
		return CSP_ERR_NOTSUP;
#endif
	}

	packet->id.dst = dest;
	packet->id.dport = dport;
	packet->id.src = my_address;
	packet->id.sport = src_port;
	packet->id.pri = prio;

	if (csp_send_direct(packet->id, packet, timeout) != CSP_ERR_NONE)
		return CSP_ERR_NOTSUP;
	
	return CSP_ERR_NONE;

}

int csp_sendto_reply(csp_packet_t * request_packet, csp_packet_t * reply_packet, uint32_t opts, uint32_t timeout) {
	if (request_packet == NULL)
		return CSP_ERR_INVAL;

	return csp_sendto(request_packet->id.pri, request_packet->id.src, request_packet->id.sport, request_packet->id.dport, opts, reply_packet, timeout);
}
