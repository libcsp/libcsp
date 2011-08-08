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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* CSP includes */
#include <csp/csp.h>
#include <csp/csp_endian.h>
#include <csp/interfaces/csp_if_lo.h>

#include "arch/csp_thread.h"
#include "arch/csp_queue.h"
#include "arch/csp_semaphore.h"
#include "arch/csp_time.h"
#include "arch/csp_malloc.h"

#include "crypto/csp_hmac.h"
#include "crypto/csp_xtea.h"
#include "csp_crc32.h"

#include "csp_io.h"
#include "csp_port.h"
#include "csp_conn.h"
#include "csp_route.h"
#include "transport/csp_transport.h"

/** Static local variables */
unsigned char my_address;

#if CSP_USE_PROMISC
extern csp_queue_handle_t csp_promisc_queue;
#endif

void csp_init(unsigned char address) {

    /* Initialize CSP */
    my_address = address;
	csp_conn_init();
	csp_port_init();
	csp_route_table_init();

	/* Generate CRC32 table */
#if CSP_ENABLE_CRC32
	csp_crc32_gentab();
#endif

	/* Register loopback route */
	csp_route_set(address, &csp_if_lo, CSP_NODE_MAC);

}

csp_socket_t * csp_socket(uint32_t opts) {
    
    /* Validate socket options */
	if ((opts & CSP_SO_RDPREQ) && !CSP_USE_RDP) {
		csp_debug(CSP_ERROR, "Attempt to create socket that requires RDP, but CSP was compiled without RDP support\r\n");
		return NULL;
	} else if ((opts & CSP_SO_XTEAREQ) && !CSP_ENABLE_XTEA) {
		csp_debug(CSP_ERROR, "Attempt to create socket that requires XTEA, but CSP was compiled without XTEA support\r\n");
		return NULL;
	} else if ((opts & CSP_SO_HMACREQ) && !CSP_ENABLE_HMAC) {
		csp_debug(CSP_ERROR, "Attempt to create socket that requires HMAC, but CSP was compiled without HMAC support\r\n");
		return NULL;
	} else if ((opts & CSP_SO_CRC32REQ) && !CSP_ENABLE_CRC32) {
		csp_debug(CSP_ERROR, "Attempt to create socket that requires CRC32, but CSP was compiled without CRC32 support\r\n");
		return NULL;
	} else if (opts & ~(CSP_SO_RDPREQ | CSP_SO_XTEAREQ | CSP_SO_HMACREQ | CSP_SO_CRC32REQ | CSP_SO_CONN_LESS)) {
		csp_debug(CSP_ERROR, "Invalid socket option\r\n");
		return NULL;
	}

	/* Use CSP buffers instead? */
    csp_socket_t * sock = csp_malloc(sizeof(csp_socket_t));
    if (sock == NULL)
    	return NULL;

    /* If connection less, init the queue to a pre-defined size
     * if not, the user must init the queue using csp_listen */
    if (opts & CSP_SO_CONN_LESS) {
    	sock->queue = csp_queue_create(CSP_CONN_QUEUE_LENGTH, sizeof(csp_packet_t *));
    	if (sock->queue == NULL)
    		return NULL;
    } else {
    	sock->queue = NULL;
    }
	sock->opts = opts;

	return sock;

}

csp_conn_t * csp_accept(csp_socket_t * sock, unsigned int timeout) {

    if (sock == NULL)
        return NULL;

    if (sock->queue == NULL)
    	return NULL;

	csp_conn_t * conn;
    if (csp_queue_dequeue(sock->queue, &conn, timeout) == CSP_QUEUE_OK)
        return conn;

	return NULL;

}

csp_packet_t * csp_read(csp_conn_t * conn, unsigned int timeout) {

	if ((conn == NULL) || (conn->state != CONN_OPEN))
		return NULL;

	csp_packet_t * packet = NULL;
    csp_queue_dequeue(conn->rx_queue, &packet, timeout);

	return packet;

}

int csp_send_direct(csp_id_t idout, csp_packet_t * packet, unsigned int timeout) {

	if (packet == NULL) {
		csp_debug(CSP_ERROR, "csp_send_direct called with NULL packet\r\n");
		goto err;
	}

	csp_route_t * ifout = csp_route_if(idout.dst);

	if ((ifout == NULL) || (ifout->interface == NULL) || (ifout->interface->nexthop == NULL)) {
		csp_debug(CSP_ERROR, "No route to host: %#08x\r\n", idout.ext);
		goto err;
	}

	csp_debug(CSP_PACKET, "Sending packet size %u from %u to %u port %u via interface %s\r\n", packet->length, idout.src, idout.dst, idout.dport, ifout->interface->name);
	
#if CSP_USE_PROMISC
    /* Loopback traffic is added to promisc queue by the router */
    if (idout.dst != my_address) {
        packet->id.ext = idout.ext;
        csp_promisc_add(packet, csp_promisc_queue);
    }
#endif

	/* Only encrypt packets from the current node */
    if (idout.src == my_address) {
		/* Append HMAC */
		if (idout.flags & CSP_FHMAC) {
#if CSP_ENABLE_HMAC
			/* Calculate and add HMAC */
			if (csp_hmac_append(packet) != 0) {
				/* HMAC append failed */
				csp_debug(CSP_WARN, "HMAC append failed!\r\n");
				goto tx_err;
			}
#else
			csp_debug(CSP_WARN, "Attempt to send packet with HMAC, but CSP was compiled without HMAC support. Discarding packet\r\n");
			goto tx_err;
#endif
		}

		/* Append CRC32 */
		if (idout.flags & CSP_FCRC32) {
#if CSP_ENABLE_CRC32
			/* Calculate and add CRC32 */
			if (csp_crc32_append(packet) != 0) {
				/* CRC32 append failed */
				csp_debug(CSP_WARN, "CRC32 append failed!\r\n");
				goto tx_err;
			}
#else
			csp_debug(CSP_WARN, "Attempt to send packet with CRC32, but CSP was compiled without CRC32 support. Discarding packet\r\n");
			goto tx_err;
#endif
		}

		if (idout.flags & CSP_FXTEA) {
#if CSP_ENABLE_XTEA
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
				csp_debug(CSP_WARN, "Encryption failed! Discarding packet\r\n");
				goto tx_err;
			}

			packet->length += sizeof(nonce_n);
#else
			csp_debug(CSP_WARN, "Attempt to send XTEA encrypted packet, but CSP was compiled without XTEA support. Discarding packet\r\n");
			goto tx_err;
#endif
		}
	}

    /* Copy identifier to packet */
    packet->id.ext = idout.ext;

    /* Store length before passing to interface */
    uint32_t b = packet->length;

	if ((*ifout->interface->nexthop)(packet, timeout) != 1)
		goto tx_err;

	ifout->interface->tx++;
	ifout->interface->txbytes += b;
	return 1;

tx_err:
	ifout->interface->tx_error++;
err:
	return 0;

}

int csp_send(csp_conn_t * conn, csp_packet_t * packet, unsigned int timeout) {

	if ((conn == NULL) || (packet == NULL) || (conn->state != CONN_OPEN)) {
		csp_debug(CSP_ERROR, "Invalid call to csp_send\r\n");
		return 0;
	}

#if CSP_USE_RDP
	if (conn->idout.flags & CSP_FRDP) {
		if (csp_rdp_send(conn, packet, timeout) == 0) {
			csp_route_t * ifout = csp_route_if(conn->idout.dst);
			if (ifout != NULL && ifout->interface != NULL)
				ifout->interface->tx_error++;
			csp_debug(CSP_WARN, "RPD send failed\r\n!");
			return 0;
		}
	}
#endif

	return csp_send_direct(conn->idout, packet, timeout);

}

int csp_transaction_persistent(csp_conn_t * conn, unsigned int timeout, void * outbuf, int outlen, void * inbuf, int inlen) {

	/* Stupid way to implement max() but more portable than macros */
	int size = outlen;
	if (inlen > outlen)
		size = inlen;

	csp_packet_t * packet = csp_buffer_get(size);
	if (packet == NULL)
		return 0;

	/* Copy the request */
	if (outlen > 0 && outbuf != NULL)
		memcpy(packet->data, outbuf, outlen);
	packet->length = outlen;

	if (!csp_send(conn, packet, timeout)) {
		csp_debug(CSP_ERROR, "Transaction send failed\r\n");
		csp_buffer_free(packet);
		return 0;
	}

	/* If no reply is expected, return now */
	if (inlen == 0)
		return 1;

	packet = csp_read(conn, timeout);
	if (packet == NULL) {
		csp_debug(CSP_WARN, "Transaction read failed\r\n");
		return 0;
	}

	if ((inlen != -1) && ((int)packet->length != inlen)) {
		csp_debug(CSP_ERROR, "Reply length %u expected %u\r\n", packet->length, inlen);
		csp_buffer_free(packet);
		return 0;
	}

	memcpy(inbuf, packet->data, packet->length);
	int length = packet->length;
	csp_buffer_free(packet);
	return length;

}

int csp_transaction(uint8_t prio, uint8_t dest, uint8_t port, unsigned int timeout, void * outbuf, int outlen, void * inbuf, int inlen) {

	csp_conn_t * conn = csp_connect(prio, dest, port, 0, 0);
	if (conn == NULL)
		return 0;

	int status = csp_transaction_persistent(conn, timeout, outbuf, outlen, inbuf, inlen);

	csp_close(conn);

	return status;

}

csp_packet_t * csp_recvfrom(csp_socket_t * socket, unsigned int timeout) {

	if ((socket == NULL) || (!(socket->opts & CSP_SO_CONN_LESS)))
		return NULL;

	csp_packet_t * packet = NULL;
    csp_queue_dequeue(socket->queue, &packet, timeout);

	return packet;

}

int csp_sendto(uint8_t prio, uint8_t dest, uint8_t dport, uint8_t src_port, uint32_t opts, csp_packet_t * packet, unsigned int timeout) {

	packet->id.flags = 0;

	if (opts & CSP_O_RDP) {
		csp_debug(CSP_ERROR, "Attempt to create RDP packet on connection-less socket\r\n");
		return -1;
	}

	if (opts & CSP_O_HMAC) {
#if CSP_ENABLE_HMAC
		packet->id.flags |= CSP_FHMAC;
#else
		csp_debug(CSP_ERROR, "Attempt to create HMAC authenticated packet, but CSP was compiled without HMAC support\r\n");
		return -1;
#endif
	}

	if (opts & CSP_O_XTEA) {
#if CSP_ENABLE_XTEA
		packet->id.flags |= CSP_FXTEA;
#else
		csp_debug(CSP_ERROR, "Attempt to create XTEA encrypted packet, but CSP was compiled without XTEA support\r\n");
		return -1;
#endif
	}

	if (opts & CSP_O_CRC32) {
#if CSP_ENABLE_CRC32
		packet->id.flags |= CSP_FCRC32;
#else
		csp_debug(CSP_ERROR, "Attempt to create CRC32 validated packet, but CSP was compiled without CRC32 support\r\n");
		return -1;
#endif
	}

	packet->id.dst = dest;
	packet->id.dport = dport;
	packet->id.src = my_address;
	packet->id.sport = src_port;
	packet->id.pri = prio;

	if (csp_send_direct(packet->id, packet, timeout) == 0) {
		return -1;
	} else {
		return 0;
	}

}
