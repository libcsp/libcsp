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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* CSP includes */
#include <csp/csp.h>

#include "arch/csp_thread.h"
#include "arch/csp_queue.h"
#include "arch/csp_semaphore.h"
#include "arch/csp_time.h"
#include "arch/csp_malloc.h"

#include "csp_io.h"
#include "csp_port.h"
#include "csp_conn.h"
#include "csp_route.h"
#include "csp_if_lo.h"

/** Static local variables */
unsigned char my_address;

/** csp_init
 * Start up the can-space protocol
 * @param address The CSP node address
 */
void csp_init(unsigned char address) {

    /* Initialize CAN */
    my_address = address;
	csp_conn_init();
	csp_port_init();
	csp_route_table_init();

	/* Register loopback route */
	csp_route_set("LOOP", address, csp_lo_tx);

}

/** csp_socket
 * Create CSP socket endpoint
 */
csp_socket_t * csp_socket(void) {
    
    // Use CSP buffers instead?
    csp_socket_t * sock = csp_malloc(sizeof(csp_socket_t));
    if (sock != NULL)
        sock->conn_queue = NULL;

    return sock;

}

/**
 * Wait for a new connection on a socket created by csp_socket
 * @param sock Socket to accept connections on
 * @param timeout use portMAX_DELAY for infinite timeout
 * @return Return pointer to csp_conn_t or NULL if timeout was reached
 */
csp_conn_t * csp_accept(csp_socket_t * sock, int timeout) {

    if (sock == NULL)
        return NULL;

    if (sock->conn_queue == NULL)
    	return NULL;

	csp_conn_t * conn;
    if (csp_queue_dequeue(sock->conn_queue, &conn, timeout) == CSP_QUEUE_OK)
        return conn;

	return NULL;

}

/**
 * Read data from a connection
 * This fuction uses the RX queue of a connection to receive a packet
 * If no packet is available and a timeout has been specified
 * The call will blovk.
 * Do NOT call this from ISR
 * @param conn pointer to connection
 * @param timeout timeout in ticks, use portMAX_DELAY for infinite blocking time
 * @return Returns pointer to csp_packet_t, which you MUST free yourself, either by calling csp_buffer_free() or reusing the buffer for a new csp_send.
 */
csp_packet_t * csp_read(csp_conn_t * conn, int timeout) {

	if (conn == NULL)
		return NULL;

	csp_packet_t * packet = NULL;
    csp_queue_dequeue(conn->rx_queue, &packet, timeout);

	return packet;

}

/**
 * Function to transmit a frame without an existing connection structure.
 * This function is used for stateless transmissions
 * @param idout 32bit CSP identifyer
 * @param packet pointer to packet,
 * @param timeout a timeout to wait for TX to complete. NOTE: not all underlying drivers supports flow-control.
 * @return returns 1 if successful and 0 otherwise. you MUST free the frame yourself if the transmission was not successful.
 */
int csp_send_direct(csp_id_t idout, csp_packet_t * packet, int timeout) {

	if (packet == NULL) {
		csp_debug("Invalid call to csp_send_direct\r\n");
		return 0;
	}

	csp_iface_t * ifout = csp_route_if(idout.dst);

	if ((ifout == NULL) || (*ifout->nexthop == NULL)) {
		csp_debug("No route to host: %#08x\r\n", idout.ext);
		return 0;
	}

	csp_debug("Sending packet from %u to %u port %u via interface %s\r\n", idout.src, idout.dst, idout.dport, ifout->name);

	return (*ifout->nexthop)(idout, packet, timeout);

}

/**
 * Send a packet on an already established connection
 * @param conn pointer to connection
 * @param packet pointer to packet,
 * @param timeout a timeout to wait for TX to complete. NOTE: not all underlying drivers supports flow-control.
 * @return returns 1 if successful and 0 otherwise. you MUST free the frame yourself if the transmission was not successful.
 */
int csp_send(csp_conn_t* conn, csp_packet_t * packet, int timeout) {

	if (conn == NULL) {
		csp_debug("Invalid call to csp_send\r\n");
		return 0;
	}

	return csp_send_direct(conn->idout, packet, timeout);

}

/**
 * Use an existing connection to perform a transaction,
 * This is only possible if the next packet is on the same port and destination!
 * @param conn pointer to connection structure
 * @param timeout timeout in ms
 * @param outbuf pointer to outgoing data buffer
 * @param outlen length of request to send
 * @param inbuf pointer to incoming data buffer
 * @param inlen length of expected reply, -1 for unknown size (note inbuf MUST be large enough)
 * @return
 */
int csp_transaction_persistent(csp_conn_t * conn, int timeout, void * outbuf, int outlen, void * inbuf, int inlen) {

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

	if (!csp_send(conn, packet, 0)) {
		printf("Send failed\r\n");
		csp_buffer_free(packet);
		return 0;
	}

	/* If no reply is expected, return now */
	if (inlen == 0)
		return 1;

	packet = csp_read(conn, timeout);
	if (packet == NULL) {
		printf("Read failed\r\n");
		return 0;
	}

	if ((inlen != -1) && (packet->length != inlen)) {
		printf("Reply length %u expected %u\r\n", packet->length, inlen);
		csp_buffer_free(packet);
		return 0;
	}

	memcpy(inbuf, packet->data, packet->length);
	int length = packet->length;
	csp_buffer_free(packet);
	return length;

}


/**
 * Perform an entire request/reply transaction
 * Copies both input buffer and reply to output buffeer.
 * Also makes the connection and closes it again
 * @param prio CSP Prio
 * @param dest CSP Dest
 * @param port CSP Port
 * @param timeout timeout in ms
 * @param outbuf pointer to outgoing data buffer
 * @param outlen length of request to send
 * @param inbuf pointer to incoming data buffer
 * @param inlen length of expected reply, -1 for unknown size (note inbuf MUST be large enough)
 * @return Return 1 or reply size if successful, 0 if error or incoming length does not match or -1 if timeout was reached
 */
int csp_transaction(uint8_t prio, uint8_t dest, uint8_t port, int timeout, void * outbuf, int outlen, void * inbuf, int inlen) {

	csp_conn_t * conn = csp_connect(prio, dest, port);
	if (conn == NULL)
		return 0;

	int status = csp_transaction_persistent(conn, timeout, outbuf, outlen, inbuf, inlen);

	csp_close(conn);

	return status;

}
