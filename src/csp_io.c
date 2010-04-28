#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

#include <csp/csp.h>

//#define DEBUG

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
	//csp_port_callback(CSP_PING, &csp_service_ping);
	//csp_port_callback(CSP_PS, &csp_service_ps);
	//csp_port_callback(CSP_MEMFREE, &csp_service_memfree);
	//csp_port_callback(CSP_REBOOT, &csp_service_reboot);
	csp_route_table_init();

	/* Register loopback route */
	extern int csp_lo_tx(csp_id_t idout, csp_packet_t * packet, unsigned int timeout);
	csp_route_set("LOOP", address, csp_lo_tx);

}

/**
 * Listen on a queue created by csp_port_listener
 * @param connQueue
 * @param timeout use portMAX_DELAY for infinite timeout
 * @return Return pointer to conn_t or NULL if timeout was reached
 */
conn_t * csp_listen(xQueueHandle connQueue, int timeout) {

	conn_t * conn;
	if (xQueueReceive(connQueue, &conn, timeout) == pdPASS)
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
csp_packet_t * csp_read(conn_t *conn, int timeout) {

	if (conn == NULL)
		return NULL;

	csp_packet_t * packet = NULL;

	xQueueReceive(conn->rxQueue, &packet, timeout);

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

	csp_iface_t * ifout = csp_route_if(idout.dst);

	if ((ifout == NULL) || (*ifout->nexthop == NULL)) {
#ifdef DEBUG
		printf("No route to host: 0x%08lX\r\n", idout.ext);
#endif
		return 0;
	}

#ifdef DEBUG
	printf("Sending packet from %u to %u port %u via interface %s\r\n", idout.src, idout.dst, idout.dport, ifout->name);
#endif

	return (*ifout->nexthop)(idout, packet, timeout);

}

/**
 * Send a packet on an already established connection
 * @param conn pointer to connection
 * @param packet pointer to packet,
 * @param timeout a timeout to wait for TX to complete. NOTE: not all underlying drivers supports flow-control.
 * @return returns 1 if successful and 0 otherwise. you MUST free the frame yourself if the transmission was not successful.
 */
int csp_send(conn_t* conn, csp_packet_t * packet, int timeout) {
	if ((conn == NULL) || (packet == NULL)) {
		printf("Invalid call to csp_send\r\n");
		return 0;
	}

	return csp_send_direct(conn->idout, packet, timeout);

}

/**
 * Perform an entire request/reply transaction
 * Copies both input buffer and reply to output buffeer.
 * @param prio CSP Prio
 * @param dest CSP Dest
 * @param port CSP Port
 * @param outbuf pointer to outgoing data buffer
 * @param outlen length of request to send
 * @param inbuf pointer to incoming data buffer
 * @param inlen length of expected reply, -1 for unknown size (note inbuf MUST be large enough)
 * @return Return 1 or reply size if successfull, 0 if error or incoming length does not match
 */
int csp_transaction(uint8_t prio, uint8_t dest, uint8_t port, int timeout, void * outbuf, int outlen, void * inbuf, int inlen) {

	conn_t * conn = csp_connect(prio, dest, port);
	if (conn == NULL)
		return 0;

	csp_packet_t * packet = csp_buffer_get(sizeof(csp_packet_t));
	if (packet == NULL) {
		csp_conn_close(conn);
		return 0;
	}

	/* Copy the request */
	if (outlen > 0 && outbuf != NULL)
		memcpy(packet->data, outbuf, outlen);
	packet->length = outlen;

	if (!csp_send(conn, packet, 0)) {
		printf("Send failed\r\n");
		csp_buffer_free(packet);
		csp_conn_close(conn);
		return 0;
	}

	/* If no reply is expected, return now */
	if (inlen == 0) {
		csp_conn_close(conn);
		return 1;
	}

	packet = csp_read(conn, timeout);
	if (packet == NULL) {
		printf("Read failed\r\n");
		csp_conn_close(conn);
		return 0;
	}

	if ((packet->length != inlen) && (inlen != -1)) {
		printf("Reply length %u expected %u\r\n", packet->length, inlen);
		csp_buffer_free(packet);
		csp_conn_close(conn);
		return 0;
	}

	memcpy(inbuf, packet->data, packet->length);
	int length = packet->length;
	csp_buffer_free(packet);
	csp_conn_close(conn);
	return length;

}
