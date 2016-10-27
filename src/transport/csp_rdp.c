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

/*
 * This is a implementation of the seq/ack handling taken from the Reliable Datagram Protocol (RDP)
 * For more information read RFC 908/1151. The implementation has been extended to include support for
 * delayed acknowledgments, to improve performance over half-duplex links.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

#include <csp/csp.h>
#include <csp/csp_endian.h>
#include <csp/csp_error.h>
#include <csp/arch/csp_queue.h>
#include <csp/arch/csp_semaphore.h>
#include <csp/arch/csp_malloc.h>
#include <csp/arch/csp_time.h>
#include "../csp_port.h"
#include "../csp_conn.h"
#include "../csp_io.h"
#include "csp_transport.h"

#ifdef CSP_USE_RDP

#define RDP_SYN	0x01
#define RDP_ACK 0x02
#define RDP_EAK 0x04
#define RDP_RST	0x08

static uint32_t csp_rdp_window_size = 4;
static uint32_t csp_rdp_conn_timeout = 10000;
static uint32_t csp_rdp_packet_timeout = 1000;
static uint32_t csp_rdp_delayed_acks = 1;
static uint32_t csp_rdp_ack_timeout = 1000 / 4;
static uint32_t csp_rdp_ack_delay_count = 4 / 2;

/* Used for queue calls */
static CSP_BASE_TYPE pdTrue = 1;

typedef struct __attribute__((__packed__)) {
	/* The timestamp is placed in the padding bytes */
	uint8_t padding[CSP_PADDING_BYTES - 2 * sizeof(uint32_t)];
	uint32_t quarantine;	// EACK quarantine period
	uint32_t timestamp;	// Time the message was sent
	uint16_t length;	// Length field must be just before CSP ID
	csp_id_t id;		// CSP id must be just before data
	uint8_t data[];		// This just points to the rest of the buffer, without a size indication.
} rdp_packet_t;

typedef struct __attribute__((__packed__)) {
	union __attribute__((__packed__)) {
		uint8_t flags;
		struct __attribute__((__packed__)) {
#if defined(CSP_BIG_ENDIAN) && !defined(CSP_LITTLE_ENDIAN)
			unsigned int res : 4;
			unsigned int syn : 1;
			unsigned int ack : 1;
			unsigned int eak : 1;
			unsigned int rst : 1;
#elif defined(CSP_LITTLE_ENDIAN) && !defined(CSP_BIG_ENDIAN)
			unsigned int rst : 1;
			unsigned int eak : 1;
			unsigned int ack : 1;
			unsigned int syn : 1;
			unsigned int res : 4;
#else
  #error "Must define one of CSP_BIG_ENDIAN or CSP_LITTLE_ENDIAN in csp_platform.h"
#endif
		};
	};
	uint16_t seq_nr;
	uint16_t ack_nr;
} rdp_header_t;

/**
 * RDP Headers:
 * The following functions are helper functions that handles the extra RDP
 * information that needs to be appended to all data packets.
 */
static rdp_header_t * csp_rdp_header_add(csp_packet_t * packet) {
	rdp_header_t * header = (rdp_header_t *) &packet->data[packet->length];
	packet->length += sizeof(rdp_header_t);
	memset(header, 0, sizeof(rdp_header_t));
	return header;
}

static rdp_header_t * csp_rdp_header_remove(csp_packet_t * packet) {
	rdp_header_t * header = (rdp_header_t *) &packet->data[packet->length-sizeof(rdp_header_t)];
	packet->length -= sizeof(rdp_header_t);
	return header;
}

static rdp_header_t * csp_rdp_header_ref(csp_packet_t * packet) {
	rdp_header_t * header = (rdp_header_t *) &packet->data[packet->length-sizeof(rdp_header_t)];
	return header;
}

/* Functions for comparing wrapping sequence numbers and timestamps */

/* Return 1 if seq is between start and end (both inclusive) */
static inline int csp_rdp_seq_between(uint16_t seq, uint16_t start, uint16_t end) {
	return (uint16_t)(end - start) >= (uint16_t)(seq - start);
}

/* Return 1 if seq is before cmp */
static inline int csp_rdp_seq_before(uint16_t seq, uint16_t cmp) {
	return (int16_t)(seq - cmp) < 0;
}

/* Return 1 if seq is after cmp */
static inline int csp_rdp_seq_after(uint16_t seq, uint16_t cmp) {
	return csp_rdp_seq_before(cmp, seq);
}

/* Return 1 if time is between start and end (both inclusive) */
static inline int csp_rdp_time_between(uint32_t time, uint32_t start, uint32_t end) {
	return (uint32_t)(end - start) >= (uint32_t)(time - start);
}

/* Return 1 if time is before cmp */
static inline int csp_rdp_time_before(uint32_t time, uint32_t cmp) {
	return (int32_t)(time - cmp) < 0;
}

/* Return 1 if time is after cmp */
static inline int csp_rdp_time_after(uint32_t time, uint32_t cmp) {
	return csp_rdp_time_before(cmp, time);
}

/**
 * CONTROL MESSAGES
 * The following function is used to send empty messages,
 * with ACK, SYN or RST flag.
 */
static int csp_rdp_send_cmp(csp_conn_t * conn, csp_packet_t * packet, int flags, int seq_nr, int ack_nr) {

	csp_id_t idout;

	/* Generate message */
	if (!packet) {
		packet = csp_buffer_get(20);
		if (!packet)
			return CSP_ERR_NOMEM;
		packet->length = 0;
	}

	/* Add RDP header */
	rdp_header_t * header = csp_rdp_header_add(packet);
	header->seq_nr = csp_hton16(seq_nr);
	header->ack_nr = csp_hton16(ack_nr);
	header->ack = (flags & RDP_ACK) ? 1 : 0;
	header->eak = (flags & RDP_EAK) ? 1 : 0;
	header->syn = (flags & RDP_SYN) ? 1 : 0;
	header->rst = (flags & RDP_RST) ? 1 : 0;

	/* Send copy to tx_queue, before sending packet to IF */
	if (flags & RDP_SYN) {
		rdp_packet_t * rdp_packet = csp_buffer_clone(packet);
		if (rdp_packet == NULL) return CSP_ERR_NOMEM;
		rdp_packet->timestamp = csp_get_ms();
		if (csp_queue_enqueue(conn->rdp.tx_queue, &rdp_packet, 0) != CSP_QUEUE_OK)
			csp_buffer_free(rdp_packet);
	}

	/* Send control messages with high priority */
	idout = conn->idout;
	idout.pri = conn->idout.pri < CSP_PRIO_HIGH ? conn->idout.pri : CSP_PRIO_HIGH;

	/* Send packet to IF */
	csp_iface_t * ifout = csp_rtable_find_iface(idout.dst);
	if (csp_send_direct(idout, packet, ifout, 0) != CSP_ERR_NONE) {
		csp_log_error("INTERFACE ERROR: not possible to send");
		csp_buffer_free(packet);
		return CSP_ERR_BUSY;
	}

	/* Update last ACK time stamp */
	if (flags & RDP_ACK) {
		conn->rdp.rcv_lsa = ack_nr;
		conn->rdp.ack_timestamp = csp_get_ms();
	}

	return CSP_ERR_NONE;

}

/**
 * EXTENDED ACKNOWLEDGEMENTS
 * The following function sends an extended ACK packet
 */
static int csp_rdp_send_eack(csp_conn_t * conn) {

	/* Allocate message */
	csp_packet_t * packet_eack = csp_buffer_get(100);
	if (packet_eack == NULL) return CSP_ERR_NOMEM;
	packet_eack->length = 0;

	/* Loop through RX queue */
	int i, count;
	csp_packet_t * packet;
	count = csp_queue_size(conn->rdp.rx_queue);
	for (i = 0; i < count; i++) {

		if (csp_queue_dequeue_isr(conn->rdp.rx_queue, &packet, &pdTrue) != CSP_QUEUE_OK) {
			csp_log_error("Cannot dequeue from rx_queue in queue deliver");
			break;
		}

		/* Add seq nr to EACK packet */
		rdp_header_t * header = csp_rdp_header_ref(packet);
		packet_eack->data16[packet_eack->length/sizeof(uint16_t)] = csp_hton16(header->seq_nr);
		packet_eack->length += sizeof(uint16_t);
		csp_log_protocol("Added EACK nr %u", header->seq_nr);

		/* Requeue */
		csp_queue_enqueue_isr(conn->rdp.rx_queue, &packet, &pdTrue);

	}

	return csp_rdp_send_cmp(conn, packet_eack, RDP_ACK | RDP_EAK, conn->rdp.snd_nxt, conn->rdp.rcv_cur);

}


/**
 * SYN Packet
 * The following function sends a SYN packet
 */
static int csp_rdp_send_syn(csp_conn_t * conn) {

	/* Allocate message */
	csp_packet_t * packet = csp_buffer_get(100);
	if (packet == NULL) return CSP_ERR_NOMEM;

	/* Generate contents */
	packet->data32[0] = csp_hton32(csp_rdp_window_size);
	packet->data32[1] = csp_hton32(csp_rdp_conn_timeout);
	packet->data32[2] = csp_hton32(csp_rdp_packet_timeout);
	packet->data32[3] = csp_hton32(csp_rdp_delayed_acks);
	packet->data32[4] = csp_hton32(csp_rdp_ack_timeout);
	packet->data32[5] = csp_hton32(csp_rdp_ack_delay_count);
	packet->length = 6 * sizeof(uint32_t);

	return csp_rdp_send_cmp(conn, packet, RDP_SYN, conn->rdp.snd_iss, 0);

}

static inline int csp_rdp_receive_data(csp_conn_t * conn, csp_packet_t * packet) {

	/* Remove RDP header before passing to userspace */
	csp_rdp_header_remove(packet);

	/* Enqueue data */
	if (csp_conn_enqueue_packet(conn, packet) < 0) {
		csp_log_warn("Conn RX buffer full");
		return CSP_ERR_NOBUFS;
	}

	return CSP_ERR_NONE;

}

static inline void csp_rdp_rx_queue_flush(csp_conn_t * conn) {

	/* Loop through RX queue */
	int i, count;
	csp_packet_t * packet;

front:
	count = csp_queue_size(conn->rdp.rx_queue);
	for (i = 0; i < count; i++) {

		if (csp_queue_dequeue_isr(conn->rdp.rx_queue, &packet, &pdTrue) != CSP_QUEUE_OK) {
			csp_log_error("Cannot dequeue from rx_queue in queue deliver");
			break;
		}

		rdp_header_t * header = csp_rdp_header_ref(packet);
		csp_log_protocol("RX Queue deliver matching Element, seq %u", header->seq_nr);

		/* If the matching packet was found: */
		if (header->seq_nr == (uint16_t)(conn->rdp.rcv_cur + 1)) {
			csp_log_protocol("Deliver seq %u", header->seq_nr);
			csp_rdp_receive_data(conn, packet);
			conn->rdp.rcv_cur++;
			/* Loop from first element again */
			goto front;

		/* Otherwise, requeue */
		} else {
			csp_queue_enqueue_isr(conn->rdp.rx_queue, &packet, &pdTrue);
		}

	}

}

static inline bool csp_rdp_seq_in_rx_queue(csp_conn_t * conn, uint16_t seq_nr) {

	/* Loop through RX queue */
	int i, count;
	rdp_packet_t * packet;
	count = csp_queue_size(conn->rdp.rx_queue);
	for (i = 0; i < count; i++) {

		if (csp_queue_dequeue_isr(conn->rdp.rx_queue, &packet, &pdTrue) != CSP_QUEUE_OK) {
			csp_log_error("Cannot dequeue from rx_queue in queue exists");
			break;
		}

		csp_queue_enqueue_isr(conn->rdp.rx_queue, &packet, &pdTrue);

		rdp_header_t * header = csp_rdp_header_ref((csp_packet_t *) packet);
		csp_log_protocol("RX Queue exists matching Element, seq %u", header->seq_nr);

		/* If the matching packet was found, deliver */
		if (header->seq_nr == seq_nr) {
			csp_log_protocol("We have a match");
			return true;
		}

	}

	return false;

}

static inline int csp_rdp_rx_queue_add(csp_conn_t * conn, csp_packet_t * packet, uint16_t seq_nr) {

	if (csp_rdp_seq_in_rx_queue(conn, seq_nr))
		return CSP_QUEUE_ERROR;
	return csp_queue_enqueue_isr(conn->rdp.rx_queue, &packet, &pdTrue);

}

static void csp_rdp_flush_eack(csp_conn_t * conn, csp_packet_t * eack_packet) {

	/* Loop through TX queue */
	int i, j, count;
	rdp_packet_t * packet;
	count = csp_queue_size(conn->rdp.tx_queue);
	for (i = 0; i < count; i++) {

		if (csp_queue_dequeue(conn->rdp.tx_queue, &packet, 0) != CSP_QUEUE_OK) {
			csp_log_error("Cannot dequeue from tx_queue in flush EACK");
			break;
		}

		rdp_header_t * header = csp_rdp_header_ref((csp_packet_t *) packet);
		csp_log_protocol("EACK compare element, time %u, seq %u", packet->timestamp, csp_ntoh16(header->seq_nr));

		/* Look for this element in EACKs */
		int match = 0;
		for (j = 0; j < (int)((eack_packet->length - sizeof(rdp_header_t)) / sizeof(uint16_t)); j++) {
			if (csp_ntoh16(eack_packet->data16[j]) == csp_ntoh16(header->seq_nr))
				match = 1;

			/* Enable this if you want EACK's to trigger retransmission */
			if (csp_ntoh16(eack_packet->data16[j]) > csp_ntoh16(header->seq_nr)) {
				uint32_t time_now = csp_get_ms();
				if (csp_rdp_time_after(time_now, packet->quarantine)) {
					packet->timestamp = time_now - conn->rdp.packet_timeout - 1;
					packet->quarantine = time_now +	conn->rdp.packet_timeout / 2;
				}
			}
		}

		if (match == 0) {
			/* If not found, put back on tx queue */
			csp_queue_enqueue(conn->rdp.tx_queue, &packet, 0);
		} else {
			/* Found, free */
			csp_log_protocol("TX Element %u freed", csp_ntoh16(header->seq_nr));
			csp_buffer_free(packet);
		}

	}

}

static inline bool csp_rdp_should_ack(csp_conn_t * conn) {

	/* If delayed ACKs are not used, always ACK */
	if (!conn->rdp.delayed_acks) {
		if (conn->rdp.rcv_lsa != conn->rdp.rcv_cur) {
			return true;
		} else {
			return false;
		}
	}

	/* ACK if time since last ACK is greater than ACK timeout */
	uint32_t time_now = csp_get_ms();
	if (csp_rdp_time_after(time_now, conn->rdp.ack_timestamp + conn->rdp.ack_timeout))
		return true;

	/* ACK if number of unacknowledged packets is greater than delay count */
	if (csp_rdp_seq_after(conn->rdp.rcv_cur, conn->rdp.rcv_lsa + conn->rdp.ack_delay_count))
		return true;

	return false;

}

void csp_rdp_flush_all(csp_conn_t * conn) {

	if ((conn == NULL) || conn->rdp.tx_queue == NULL) {
		csp_log_error("Null pointer passed to rdp flush all");
		return;
	}

	rdp_packet_t * packet;

	/* Empty TX queue */
	while (csp_queue_dequeue_isr(conn->rdp.tx_queue, &packet, &pdTrue) == CSP_QUEUE_OK) {
		if (packet != NULL) {
			csp_log_protocol("Flush TX Element, time %u, seq %u", packet->timestamp, csp_ntoh16(csp_rdp_header_ref((csp_packet_t *) packet)->seq_nr));
			csp_buffer_free(packet);
		}
	}

	/* Empty RX queue */
	while (csp_queue_dequeue_isr(conn->rdp.rx_queue, &packet, &pdTrue) == CSP_QUEUE_OK) {
		if (packet != NULL) {
			csp_log_protocol("Flush RX Element, time %u, seq %u", packet->timestamp, csp_ntoh16(csp_rdp_header_ref((csp_packet_t *) packet)->seq_nr));
			csp_buffer_free(packet);
		}
	}

}

int csp_rdp_check_ack(csp_conn_t * conn) {

	/* Check all RX queues for spare capacity */
	int prio, avail = 1;
	for (prio = 0; prio < CSP_RX_QUEUES; prio++) {
		if (CSP_RX_QUEUE_LENGTH - csp_queue_size(conn->rx_queue[prio]) <= 2 * (int32_t)conn->rdp.window_size) {
			avail = 0;
			break;
		}
	}

	/* If more space available, only send after ack timeout or immediately if delay_acks is zero */
	if (avail && csp_rdp_should_ack(conn))
		csp_rdp_send_cmp(conn, NULL, RDP_ACK, conn->rdp.snd_nxt, conn->rdp.rcv_cur);

	return CSP_ERR_NONE;

}

/**
 * This function must be called with regular intervals for the
 * RDP protocol to work as expected. This takes care of closing
 * stale connections and retransmitting traffic. A good place to
 * call this function is from the CSP router task.
 */
void csp_rdp_check_timeouts(csp_conn_t * conn) {

	rdp_packet_t * packet;

	/**
	 * CONNECTION TIMEOUT:
	 * Check that connection has not timed out inside the network stack
	 * */
	uint32_t time_now = csp_get_ms();
	if (conn->socket != NULL) {
		if (csp_rdp_time_after(time_now, conn->timestamp + conn->rdp.conn_timeout)) {
			csp_log_warn("Found a lost connection, closing now");
			csp_close(conn);
			return;
		}
	}

	/**
	 * CLOSE-WAIT TIMEOUT:
	 * After waiting a while in CLOSE-WAIT, the connection should be closed.
	 */
	if (conn->rdp.state == RDP_CLOSE_WAIT) {
		if (csp_rdp_time_after(time_now, conn->timestamp + conn->rdp.conn_timeout)) {
			csp_log_protocol("CLOSE_WAIT timeout");
			csp_close(conn);
		}
		return;
	}

	/**
	 * MESSAGE TIMEOUT:
	 * Check each outgoing message for TX timeout
	 */
	int i, count;
	count = csp_queue_size(conn->rdp.tx_queue);
	for (i = 0; i < count; i++) {

		if ((csp_queue_dequeue_isr(conn->rdp.tx_queue, &packet, &pdTrue) != CSP_QUEUE_OK) || packet == NULL) {
			csp_log_warn("Cannot dequeue from tx_queue in check timeout");
			break;
		}

		/* Get header */
		rdp_header_t * header = csp_rdp_header_ref((csp_packet_t *) packet);

		/* If acked, do not retransmit */
		if (csp_rdp_seq_before(csp_ntoh16(header->seq_nr), conn->rdp.snd_una)) {
			csp_log_protocol("TX Element Free, time %u, seq %u, una %u", packet->timestamp, csp_ntoh16(header->seq_nr), conn->rdp.snd_una);
			csp_buffer_free(packet);
			continue;
		}

		/* Check timestamp and retransmit if needed */
		if (csp_rdp_time_after(time_now, packet->timestamp + conn->rdp.packet_timeout)) {
			csp_log_protocol("TX Element timed out, retransmitting seq %u", csp_ntoh16(header->seq_nr));

			/* Update to latest outgoing ACK */
			header->ack_nr = csp_hton16(conn->rdp.rcv_cur);

			/* Send copy to tx_queue */
			packet->timestamp = csp_get_ms();
			csp_packet_t * new_packet = csp_buffer_clone(packet);
			csp_iface_t * ifout = csp_rtable_find_iface(conn->idout.dst);
			if (csp_send_direct(conn->idout, new_packet, ifout, 0) != CSP_ERR_NONE) {
				csp_log_warn("Retransmission failed");
				csp_buffer_free(new_packet);
			}

		}

		/* Requeue the TX element */
		csp_queue_enqueue_isr(conn->rdp.tx_queue, &packet, &pdTrue);

	}

	/**
	 * ACK TIMEOUT:
	 * Check ACK timeouts, if we have unacknowledged segments
	 */
	csp_rdp_check_ack(conn);

	/* Wake user task if TX queue is ready for more data */
	if (conn->rdp.state == RDP_OPEN)
		if (csp_queue_size(conn->rdp.tx_queue) < (int)conn->rdp.window_size)
			if (csp_rdp_seq_before(conn->rdp.snd_nxt - conn->rdp.snd_una, conn->rdp.window_size * 2))
				csp_bin_sem_post(&conn->rdp.tx_wait);

}

void csp_rdp_new_packet(csp_conn_t * conn, csp_packet_t * packet) {

	/* Get RX header and convert to host byte-order */
	rdp_header_t * rx_header = csp_rdp_header_ref(packet);
	rx_header->ack_nr = csp_ntoh16(rx_header->ack_nr);
	rx_header->seq_nr = csp_ntoh16(rx_header->seq_nr);

	csp_log_protocol("RDP: Received in S %u: syn %u, ack %u, eack %u, "
			"rst %u, seq_nr %5u, ack_nr %5u, packet_len %u (%u)",
			conn->rdp.state, rx_header->syn, rx_header->ack, rx_header->eak,
			rx_header->rst, rx_header->seq_nr, rx_header->ack_nr,
			packet->length, packet->length - sizeof(rdp_header_t));

	/* If a RESET was received. */
	if (rx_header->rst) {

		if (rx_header->ack) {
			/* Store current ack'ed sequence number */
			conn->rdp.snd_una = rx_header->ack_nr + 1;
		}

		if (conn->rdp.state == RDP_CLOSE_WAIT || conn->rdp.state == RDP_CLOSED) {
			csp_log_protocol("RST received in CLOSE_WAIT or CLOSED. Now closing connection");
			goto discard_close;
		} else {
			csp_log_protocol("Got RESET in state %u", conn->rdp.state);

			if (rx_header->seq_nr == (uint16_t)(conn->rdp.rcv_cur + 1)) {
				csp_log_protocol("RESET in sequence, no more data incoming, reply with RESET");
				conn->rdp.state = RDP_CLOSE_WAIT;
				conn->timestamp = csp_get_ms();
				csp_rdp_send_cmp(conn, NULL, RDP_ACK | RDP_RST, conn->rdp.snd_nxt, conn->rdp.rcv_cur);
				goto discard_close;
			} else {
				csp_log_protocol("RESET out of sequence, keep connection open");
				goto discard_open;
			}
		}
	}

	/* The BIG FAT switch (state-machine) */
	switch(conn->rdp.state) {

	/**
	 * STATE == CLOSED
	 */
	case RDP_CLOSED: {

		/* No SYN flag set while in closed. Inform by sending back RST */
		if (!rx_header->syn) {
			csp_log_protocol("Not SYN received in CLOSED state. Discarding packet");
			csp_rdp_send_cmp(conn, NULL, RDP_RST, conn->rdp.snd_nxt, conn->rdp.rcv_cur);
			goto discard_close;
		}

		csp_log_protocol("RDP: SYN-Received");

		/* Setup TX seq. */
		conn->rdp.snd_iss = (uint16_t)rand();
		conn->rdp.snd_nxt = conn->rdp.snd_iss + 1;
		conn->rdp.snd_una = conn->rdp.snd_iss;

		/* Store RX seq. */
		conn->rdp.rcv_cur = rx_header->seq_nr;
		conn->rdp.rcv_irs = rx_header->seq_nr;
		conn->rdp.rcv_lsa = rx_header->seq_nr;

		/* Store RDP options */
		conn->rdp.window_size 		= csp_ntoh32(packet->data32[0]);
		conn->rdp.conn_timeout 		= csp_ntoh32(packet->data32[1]);
		conn->rdp.packet_timeout 	= csp_ntoh32(packet->data32[2]);
		conn->rdp.delayed_acks 		= csp_ntoh32(packet->data32[3]);
		conn->rdp.ack_timeout 		= csp_ntoh32(packet->data32[4]);
		conn->rdp.ack_delay_count 	= csp_ntoh32(packet->data32[5]);
		csp_log_protocol("RDP: Window Size %u, conn timeout %u, packet timeout %u",
				conn->rdp.window_size, conn->rdp.conn_timeout, conn->rdp.packet_timeout);
		csp_log_protocol("RDP: Delayed acks: %u, ack timeout %u, ack each %u packet",
				conn->rdp.delayed_acks, conn->rdp.ack_timeout, conn->rdp.ack_delay_count);

		/* Connection accepted */
		conn->rdp.state = RDP_SYN_RCVD;

		/* Send SYN/ACK */
		csp_rdp_send_cmp(conn, NULL, RDP_ACK | RDP_SYN, conn->rdp.snd_iss, conn->rdp.rcv_irs);

		goto discard_open;

	}
	break;

	/**
	 * STATE == SYN-SENT
	 */
	case RDP_SYN_SENT: {

		/* First check SYN/ACK */
		if (rx_header->syn && rx_header->ack) {

			conn->rdp.rcv_cur = rx_header->seq_nr;
			conn->rdp.rcv_irs = rx_header->seq_nr;
			conn->rdp.rcv_lsa = rx_header->seq_nr - 1;
			conn->rdp.snd_una = rx_header->ack_nr + 1;
			conn->rdp.ack_timestamp = csp_get_ms();
			conn->rdp.state = RDP_OPEN;

			csp_log_protocol("RDP: NP: Connection OPEN");

			/* Send ACK */
			csp_rdp_send_cmp(conn, NULL, RDP_ACK, conn->rdp.snd_nxt, conn->rdp.rcv_cur);

			/* Wake TX task */
			csp_bin_sem_post(&conn->rdp.tx_wait);

			goto discard_open;
		}

		/* If there was no SYN in the reply, our SYN message hit an already open connection
		 * This is handled by sending a RST.
		 * Normally this would be followed up by a new connection attempt, however
		 * we don't have a method for signaling this to the user space.
		 */
		if (rx_header->ack) {
			csp_log_error("Half-open connection found, sending RST");
			csp_rdp_send_cmp(conn, NULL, RDP_RST, conn->rdp.snd_nxt, conn->rdp.rcv_cur);
			csp_bin_sem_post(&conn->rdp.tx_wait);

			goto discard_open;
		}

		/* Otherwise we have an invalid command, such as a SYN reply to a SYN command,
		 * indicating simultaneous connections, which is not possible in the way CSP
		 * reserves some ports for server and some for clients.
		 */
		csp_log_error("Invalid reply to SYN request");
		goto discard_close;

	}
	break;

	/**
	 * STATE == OPEN
	 */
	case RDP_SYN_RCVD:
	case RDP_OPEN:
	{

		/* SYN or !ACK is invalid */
		if (rx_header->syn || !rx_header->ack) {
			if (rx_header->seq_nr != conn->rdp.rcv_irs) {
				csp_log_error("Invalid SYN or no ACK, resetting!");
				goto discard_close;
			} else {
				csp_log_protocol("Ignoring duplicate SYN packet!");
				goto discard_open;
			}
		}

		/* Check sequence number */
		if (!csp_rdp_seq_between(rx_header->seq_nr, conn->rdp.rcv_cur + 1, conn->rdp.rcv_cur + conn->rdp.window_size * 2)) {
			csp_log_protocol("Invalid sequence number! %"PRIu16" not between %"PRIu16" and %"PRIu16,
					rx_header->seq_nr, conn->rdp.rcv_cur + 1, conn->rdp.rcv_cur + 1 + conn->rdp.window_size * 2);
			/* If duplicate SYN received, send another SYN/ACK */
			if (conn->rdp.state == RDP_SYN_RCVD)
				csp_rdp_send_cmp(conn, NULL, RDP_ACK | RDP_SYN, conn->rdp.snd_iss, conn->rdp.rcv_irs);
			/* If duplicate data packet received, send EACK back */
			if (conn->rdp.state == RDP_OPEN)
				csp_rdp_send_eack(conn);

			goto discard_open;
		}

		/* Check ACK number */
		if (!csp_rdp_seq_between(rx_header->ack_nr, conn->rdp.snd_una - 1 - (conn->rdp.window_size * 2), conn->rdp.snd_nxt - 1)) {
			csp_log_error("Invalid ACK number! %u not between %u and %u",
					rx_header->ack_nr, conn->rdp.snd_una - 1 - (conn->rdp.window_size * 2), conn->rdp.snd_nxt - 1);
			goto discard_open;
		}

		/* Check SYN_RCVD ACK */
		if (conn->rdp.state == RDP_SYN_RCVD) {
			if (rx_header->ack_nr != conn->rdp.snd_iss) {
				csp_log_error("SYN-RCVD: Wrong ACK number");
				goto discard_close;
			}
			csp_log_protocol("RDP: NC: Connection OPEN");
			conn->rdp.state = RDP_OPEN;

			/* If a socket is set, this message is the first in a new connection
			 * so the connection must be queued to the socket. */
			if (conn->socket != NULL) {

				/* Try queueing */
				if (csp_queue_enqueue(conn->socket, &conn, 0) == CSP_QUEUE_FULL) {
					csp_log_error("ERROR socket cannot accept more connections");
					goto discard_close;
				}

				/* Ensure that this connection will not be posted to this socket again
				 * and remember that the connection handle has been passed to userspace
				 * by setting the socket = NULL */
				conn->socket = NULL;
			}

		}

		/* Store current ack'ed sequence number */
		conn->rdp.snd_una = rx_header->ack_nr + 1;

		/* We have an EACK */
		if (rx_header->eak) {
			if (packet->length > sizeof(rdp_header_t))
				csp_rdp_flush_eack(conn, packet);
			goto discard_open;
		}

		/* If no data, return here */
		if (packet->length <= sizeof(rdp_header_t))
			goto discard_open;

		/* If message is not in sequence, send EACK and store packet */
		if (rx_header->seq_nr != (uint16_t)(conn->rdp.rcv_cur + 1)) {
			if (csp_rdp_rx_queue_add(conn, packet, rx_header->seq_nr) != CSP_QUEUE_OK) {
				csp_log_protocol("Duplicate sequence number");
				goto discard_open;
			}
			csp_rdp_send_eack(conn);
			goto accepted_open;
		}

		/* Store sequence number before stripping RDP header */
		uint16_t seq_nr = rx_header->seq_nr;

		/* Receive data */
		if (csp_rdp_receive_data(conn, packet) != CSP_ERR_NONE)
			goto discard_open;

		/* Update last received packet */
		conn->rdp.rcv_cur = seq_nr;

		/* Only ACK the message if there is room for a full window in the RX buffer.
		 * Unacknowledged segments are ACKed by csp_rdp_check_timeouts when the buffer is
		 * no longer full. */
		csp_rdp_check_ack(conn);

		/* Flush RX queue */
		csp_rdp_rx_queue_flush(conn);

		goto accepted_open;

	}
	break;

	case RDP_CLOSE_WAIT:

		/* Ignore SYN or !ACK */
		if (rx_header->syn || !rx_header->ack) {
			csp_log_protocol("Invalid SYN or no ACK in CLOSE-WAIT");
			goto discard_open;
		}

		/* Check ACK number */
		if (!csp_rdp_seq_between(rx_header->ack_nr, conn->rdp.snd_una - 1 - (conn->rdp.window_size * 2), conn->rdp.snd_nxt - 1)) {
			csp_log_error("Invalid ACK number! %u not between %u and %u",
					rx_header->ack_nr, conn->rdp.snd_una - 1 - (conn->rdp.window_size * 2), conn->rdp.snd_nxt - 1);
			goto discard_open;
		}

		/* Store current ack'ed sequence number */
		conn->rdp.snd_una = rx_header->ack_nr + 1;

		/* Send back a reset */
		csp_rdp_send_cmp(conn, NULL, RDP_ACK | RDP_RST, conn->rdp.snd_nxt, conn->rdp.rcv_cur);

		goto discard_open;

	default:
		csp_log_error("RDP: ERROR default state!");
		goto discard_close;
	}

discard_close:
	/* If user-space has received the connection handle, wake it up,
	 * by sending a NULL pointer, user-space should close connection */
	if (conn->socket == NULL) {
		csp_log_protocol("Waiting for userspace to close");
		csp_conn_enqueue_packet(conn, NULL);
	} else {
		csp_close(conn);
	}

discard_open:
	csp_buffer_free(packet);
accepted_open:
	return;

}

int csp_rdp_connect(csp_conn_t * conn, uint32_t timeout) {

	int retry = 1;

	conn->rdp.window_size	 = csp_rdp_window_size;
	conn->rdp.conn_timeout	= csp_rdp_conn_timeout;
	conn->rdp.packet_timeout  = csp_rdp_packet_timeout;
	conn->rdp.delayed_acks	= csp_rdp_delayed_acks;
	conn->rdp.ack_timeout 	  = csp_rdp_ack_timeout;
	conn->rdp.ack_delay_count = csp_rdp_ack_delay_count;
	conn->rdp.ack_timestamp   = csp_get_ms();

retry:
	csp_log_protocol("RDP: Active connect, conn state %u", conn->rdp.state);

	if (conn->rdp.state == RDP_OPEN) {
		csp_log_error("RDP: Connection already open");
		return CSP_ERR_ALREADY;
	}

	/* Randomize ISS */
	conn->rdp.snd_iss = (uint16_t)rand();

	conn->rdp.snd_nxt = conn->rdp.snd_iss + 1;
	conn->rdp.snd_una = conn->rdp.snd_iss;

	csp_log_protocol("RDP: AC: Sending SYN");

	/* Ensure semaphore is busy, so router task can release it */
	csp_bin_sem_wait(&conn->rdp.tx_wait, 0);

	/* Send SYN message */
	conn->rdp.state = RDP_SYN_SENT;
	if (csp_rdp_send_syn(conn) != CSP_ERR_NONE)
		goto error;

	/* Wait for router task to release semaphore */
	csp_log_protocol("RDP: AC: Waiting for SYN/ACK reply...");
	int result = csp_bin_sem_wait(&conn->rdp.tx_wait, conn->rdp.conn_timeout);

	if (result == CSP_SEMAPHORE_OK) {
		if (conn->rdp.state == RDP_OPEN) {
			csp_log_protocol("RDP: AC: Connection OPEN");
			return CSP_ERR_NONE;
		} else if(conn->rdp.state == RDP_SYN_SENT) {
			if (retry) {
				csp_log_warn("RDP: Half-open connection detected, RST sent, now retrying");
				csp_rdp_flush_all(conn);
				retry = 0;
				goto retry;
			} else {
				csp_log_error("RDP: Connection stayed half-open, even after RST and retry!");
				goto error;
			}
		}
	} else {
		csp_log_protocol("RDP: AC: Connection Failed");
		goto error;
	}

error:
	conn->rdp.state = RDP_CLOSE_WAIT;
	return CSP_ERR_TIMEDOUT;

}

int csp_rdp_send(csp_conn_t * conn, csp_packet_t * packet, uint32_t timeout) {

	if (conn->rdp.state != RDP_OPEN) {
		csp_log_error("RDP: ERROR cannot send, connection reset");
		return CSP_ERR_RESET;
	}

	/* If TX window is full, wait here */
	while (csp_rdp_seq_after(conn->rdp.snd_nxt, conn->rdp.snd_una + (uint16_t)conn->rdp.window_size)) {
		csp_log_protocol("RDP: Waiting for window update before sending seq %u", conn->rdp.snd_nxt);
		csp_bin_sem_wait(&conn->rdp.tx_wait, 0);
		if ((csp_bin_sem_wait(&conn->rdp.tx_wait, conn->rdp.conn_timeout)) != CSP_SEMAPHORE_OK) {
			csp_log_error("Timeout during send");
			return CSP_ERR_TIMEDOUT;
		}
	}

	if (conn->rdp.state != RDP_OPEN) {
		csp_log_error("RDP: ERROR cannot send, connection reset");
		return CSP_ERR_RESET;
	}

	/* Add RDP header */
	rdp_header_t * tx_header = csp_rdp_header_add(packet);
	tx_header->ack_nr = csp_hton16(conn->rdp.rcv_cur);
	tx_header->seq_nr = csp_hton16(conn->rdp.snd_nxt);
	tx_header->ack = 1;

	/* Send copy to tx_queue */
	rdp_packet_t * rdp_packet = csp_buffer_clone(packet);
	if (rdp_packet == NULL) {
		csp_log_error("Failed to allocate packet buffer");
		return CSP_ERR_NOMEM;
	}

	rdp_packet->timestamp = csp_get_ms();
	rdp_packet->quarantine = 0;
	if (csp_queue_enqueue(conn->rdp.tx_queue, &rdp_packet, 0) != CSP_QUEUE_OK) {
		csp_log_error("No more space in RDP retransmit queue");
		csp_buffer_free(rdp_packet);
		return CSP_ERR_NOBUFS;
	}

	csp_log_protocol("RDP: Sending  in S %u: syn %u, ack %u, eack %u, "
				"rst %u, seq_nr %5u, ack_nr %5u, packet_len %u (%u)",
				conn->rdp.state, tx_header->syn, tx_header->ack, tx_header->eak,
				tx_header->rst, csp_ntoh16(tx_header->seq_nr), csp_ntoh16(tx_header->ack_nr),
				packet->length, packet->length - sizeof(rdp_header_t));

	conn->rdp.snd_nxt++;
	return CSP_ERR_NONE;

}

int csp_rdp_allocate(csp_conn_t * conn) {

	csp_log_buffer("RDP: Creating RDP queues for conn %p", conn);

	/* Set initial state */
	conn->rdp.state = RDP_CLOSED;
	conn->rdp.conn_timeout = csp_rdp_conn_timeout;
	conn->rdp.packet_timeout = csp_rdp_packet_timeout;

	/* Create a binary semaphore to wait on for tasks */
	if (csp_bin_sem_create(&conn->rdp.tx_wait) != CSP_SEMAPHORE_OK) {
		csp_log_error("Failed to initialize semaphore");
		return CSP_ERR_NOMEM;
	}

	/* Create TX queue */
	conn->rdp.tx_queue = csp_queue_create(CSP_RDP_MAX_WINDOW, sizeof(csp_packet_t *));
	if (conn->rdp.tx_queue == NULL) {
		csp_log_error("Failed to create TX queue for conn");
		csp_bin_sem_remove(&conn->rdp.tx_wait);
		return CSP_ERR_NOMEM;
	}

	/* Create RX queue */
	conn->rdp.rx_queue = csp_queue_create(CSP_RDP_MAX_WINDOW * 2, sizeof(csp_packet_t *));
	if (conn->rdp.rx_queue == NULL) {
		csp_log_error("Failed to create RX queue for conn");
		csp_bin_sem_remove(&conn->rdp.tx_wait);
		csp_queue_remove(conn->rdp.tx_queue);
		return CSP_ERR_NOMEM;
	}

	return CSP_ERR_NONE;

}

/**
 * @note This function may only be called from csp_close, and is therefore
 * without any checks for null pointers.
 */
int csp_rdp_close(csp_conn_t * conn) {

	if (conn->rdp.state == RDP_CLOSED)
		return CSP_ERR_NONE;

	/* If message is open, send reset */
	if (conn->rdp.state != RDP_CLOSE_WAIT) {
		conn->rdp.state = RDP_CLOSE_WAIT;
		conn->timestamp = csp_get_ms();
		csp_rdp_send_cmp(conn, NULL, RDP_ACK | RDP_RST, conn->rdp.snd_nxt, conn->rdp.rcv_cur);
		csp_log_protocol("RDP Close, sent RST on conn %p", conn);
		return CSP_ERR_AGAIN;
	}

	csp_log_protocol("RDP Close in CLOSE_WAIT, now closing");
	conn->rdp.state = RDP_CLOSED;
	return CSP_ERR_NONE;

}

/**
 * RDP Set socket options
 * Controls important parameters of the RDP protocol.
 * These settings will be applied to all new outgoing connections.
 * The settings are global, so be sure no other task are conflicting with your settings.
 */
void csp_rdp_set_opt(unsigned int window_size, unsigned int conn_timeout_ms,
		unsigned int packet_timeout_ms, unsigned int delayed_acks,
		unsigned int ack_timeout, unsigned int ack_delay_count) {
	csp_rdp_window_size = window_size;
	csp_rdp_conn_timeout = conn_timeout_ms;
	csp_rdp_packet_timeout = packet_timeout_ms;
	csp_rdp_delayed_acks = delayed_acks;
	csp_rdp_ack_timeout = ack_timeout;
	csp_rdp_ack_delay_count = ack_delay_count;
}

void csp_rdp_get_opt(unsigned int * window_size, unsigned int * conn_timeout_ms,
		unsigned int * packet_timeout_ms, unsigned int * delayed_acks,
		unsigned int * ack_timeout, unsigned int * ack_delay_count) {

	if (window_size)
		*window_size = csp_rdp_window_size;
	if (conn_timeout_ms)
		*conn_timeout_ms = csp_rdp_conn_timeout;
	if (packet_timeout_ms)
		*packet_timeout_ms = csp_rdp_packet_timeout;
	if (delayed_acks)
		*delayed_acks = csp_rdp_delayed_acks;
	if (ack_timeout)
		*ack_timeout = csp_rdp_ack_timeout;
	if (ack_delay_count)
		*ack_delay_count = csp_rdp_ack_delay_count;
}

#ifdef CSP_DEBUG
void csp_rdp_conn_print(csp_conn_t * conn) {

	if (conn == NULL)
		return;

	printf("\tRDP: State %"PRIu16", rcv %"PRIu16", snd %"PRIu16", win %"PRIu32"\r\n",
			conn->rdp.state, conn->rdp.rcv_cur, conn->rdp.snd_una, conn->rdp.window_size);

}
#endif

#endif
