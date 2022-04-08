

/*
 * This is a implementation of the seq/ack handling taken from the Reliable Datagram Protocol (RDP)
 * For more information read RFC 908/1151. The implementation has been extended to include support for
 * delayed acknowledgments, to improve performance over half-duplex links.
 */

#include "csp_rdp_queue.h"

#include <stdlib.h>
#include <string.h>
#include <endian.h>

#include <csp/csp.h>
#include <csp/csp_debug.h>
#include <csp/csp_error.h>
#include <csp/arch/csp_queue.h>
#include <csp/arch/csp_time.h>

#include "csp_port.h"
#include "csp_conn.h"
#include "csp_io.h"
#include "csp_semaphore.h"

#define RDP_SYN 0x08
#define RDP_ACK 0x04
#define RDP_EAK 0x02
#define RDP_RST 0x01

#ifndef CSP_USE_RDP_FAST_CLOSE
#define CSP_USE_RDP_FAST_CLOSE 1
#endif

#if (CSP_USE_RDP)


static uint32_t csp_rdp_window_size = 4;
static uint32_t csp_rdp_conn_timeout = 10000;
static uint32_t csp_rdp_packet_timeout = 1000;
static uint32_t csp_rdp_delayed_acks = 1;
static uint32_t csp_rdp_ack_timeout = 1000 / 4;
static uint32_t csp_rdp_ack_delay_count = 4 / 2;

typedef struct __attribute__((__packed__)) {
	uint8_t flags;
	uint16_t seq_nr;
	uint16_t ack_nr;
} rdp_header_t;

static int csp_rdp_close_internal(csp_conn_t * conn, uint8_t closed_by, bool send_rst);

/**
 * RDP Headers:
 * The following functions are helper functions that handles the extra RDP
 * information that needs to be appended to all data packets.
 */
static rdp_header_t * csp_rdp_header_add(csp_packet_t * packet) {
	rdp_header_t * header;
	if ((packet->length + sizeof(*header)) > csp_buffer_data_size()) {
		return NULL;
	}
	header = (rdp_header_t *)&packet->data[packet->length];
	packet->length += sizeof(*header);
	memset(header, 0, sizeof(*header));
	return header;
}

static rdp_header_t * csp_rdp_header_remove(csp_packet_t * packet) {
	rdp_header_t * header = (rdp_header_t *)&packet->data[packet->length - sizeof(*header)];
	packet->length -= sizeof(*header);
	return header;
}

static rdp_header_t * csp_rdp_header_ref(csp_packet_t * packet) {
	rdp_header_t * header = (rdp_header_t *)&packet->data[packet->length - sizeof(*header)];
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
// static inline int csp_rdp_time_between(uint32_t time, uint32_t start, uint32_t end) {
//	return (uint32_t)(end - start) >= (uint32_t)(time - start);
// }

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

	/* Generate message */
	if (!packet) {
		packet = csp_buffer_get(20);
		if (!packet)
			return CSP_ERR_NOMEM;
		packet->length = 0;
	}

	/* Add RDP header */
	rdp_header_t * header = csp_rdp_header_add(packet);
	if (header == NULL) {
		csp_rdp_error("RDP %p: No space for RDP header (cmp)", conn);
		csp_buffer_free(packet);
		return CSP_ERR_NOMEM;
	}
	header->seq_nr = htobe16(seq_nr);
	header->ack_nr = htobe16(ack_nr);

	/* Add a bit of ephemeral data to avoid CMP's to be deduplicated */
	static uint8_t csp_rdp_incr = 0;
	//header->flags = flags;
	header->flags |= csp_rdp_incr++ << 4 | flags;

	/* Send copy to tx_queue, before sending packet to IF */
	if (flags & RDP_SYN) {
		csp_packet_t * rdp_packet = csp_buffer_clone(packet);
		if (rdp_packet == NULL) return CSP_ERR_NOMEM;
		rdp_packet->timestamp_tx = csp_get_ms();
		csp_rdp_queue_tx_add(conn, rdp_packet);
	}

	/* Send control messages with high priority */
	csp_id_t idout = conn->idout;
	idout.pri = conn->idout.pri < CSP_PRIO_HIGH ? conn->idout.pri : CSP_PRIO_HIGH;

	csp_rdp_protocol("RDP %p: Send CMP S %u: syn %u, ack %u, eack %u, rst %u, seq_nr %5u, ack_nr %5u, packet_len %u (%u)\n",
					 conn, conn->rdp.state, ((header->flags & RDP_SYN) != 0), ((header->flags & RDP_ACK) != 0), ((header->flags & RDP_EAK) != 0),
					 ((header->flags & RDP_RST) != 0), be16toh(header->seq_nr), be16toh(header->ack_nr),
					 packet->length, (unsigned int)(packet->length - sizeof(rdp_header_t)));

	/* Send packet to IF */
	csp_send_direct(&idout, packet, NULL);

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
	count = csp_rdp_queue_rx_size();
	unsigned int space_available = 100 - (packet_eack->length + sizeof(rdp_header_t));

	for (i = 0; i < count; i++) {

		packet = csp_rdp_queue_rx_get(conn);
		if (packet == NULL) {
			break;
		}

		/* Add seq nr to EACK packet */
		rdp_header_t * header = csp_rdp_header_ref(packet);
		if (space_available >= sizeof(uint16_t)) {
			packet_eack->data16[packet_eack->length / sizeof(uint16_t)] = htobe16(header->seq_nr);
			packet_eack->length += sizeof(uint16_t);
			space_available -= sizeof(uint16_t);
			csp_rdp_protocol("RDP %p: Added EACK nr %u\n", conn, header->seq_nr);
		} else {
			csp_rdp_protocol("RDP %p: Skipping EACK nr %u\n", conn, header->seq_nr);
		}

		/* Requeue */
		csp_rdp_queue_rx_add(conn, packet);
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
	packet->data32[0] = htobe32(csp_rdp_window_size);
	packet->data32[1] = htobe32(csp_rdp_conn_timeout);
	packet->data32[2] = htobe32(csp_rdp_packet_timeout);
	packet->data32[3] = htobe32(csp_rdp_delayed_acks);
	packet->data32[4] = htobe32(csp_rdp_ack_timeout);
	packet->data32[5] = htobe32(csp_rdp_ack_delay_count);
	packet->length = 6 * sizeof(uint32_t);

	return csp_rdp_send_cmp(conn, packet, RDP_SYN, conn->rdp.snd_iss, 0);
}

static inline int csp_rdp_receive_data(csp_conn_t * conn, csp_packet_t * packet) {

	/* Remove RDP header before passing to userspace */
	csp_rdp_header_remove(packet);

	/* Enqueue data */
	if (csp_conn_enqueue_packet(conn, packet) < 0) {
		csp_dbg_conn_ovf++;
		csp_rdp_error("RDP %p: Conn RX buffer full\n", conn);
		return CSP_ERR_NOBUFS;
	}

	return CSP_ERR_NONE;
}

static inline void csp_rdp_rx_queue_flush(csp_conn_t * conn) {

	/* Loop through RX queue */
	int i, count;
	csp_packet_t * packet;

front:
	count = csp_rdp_queue_rx_size();
	for (i = 0; i < count; i++) {

		/* Check there is room in the RX queue:
		 * We don't hold a lock on the queue, so we require at least two spaces to be free
		 * to hopefully avoid posting packets on a full queue */
		if (csp_queue_free(conn->rx_queue) <= 2)
			return;

		packet = csp_rdp_queue_rx_get(conn);
		if (packet == NULL) {
			break;
		}

		rdp_header_t * header = csp_rdp_header_ref(packet);
		csp_rdp_protocol("RDP %p: RX Queue deliver matching Element, seq %u\n", conn, header->seq_nr);

		/* If the matching packet was found: */
		if (header->seq_nr == (uint16_t)(conn->rdp.rcv_cur + 1)) {
			csp_rdp_protocol("RDP %p: Deliver seq %u", conn, header->seq_nr);
			if (csp_rdp_receive_data(conn, packet) != CSP_ERR_NONE) {
				csp_rdp_error("RDP lost packet internally, stream corrupted!\n");
				csp_buffer_free(packet);
			}
			conn->rdp.rcv_cur++;

			/* Loop from first element again */
			goto front;

			/* Otherwise, requeue */
		} else {
			csp_rdp_queue_rx_add(conn, packet);
		}
	}
}

static inline bool csp_rdp_seq_in_rx_queue(csp_conn_t * conn, uint16_t seq_nr) {

	/* Loop through RX queue */
	int i, count;
	csp_packet_t * packet;
	count = csp_rdp_queue_rx_size();
	for (i = 0; i < count; i++) {

		packet = csp_rdp_queue_rx_get(conn);
		if (packet == NULL) {
			break;
		}

		csp_rdp_queue_rx_add(conn, packet);

		rdp_header_t * header = csp_rdp_header_ref((csp_packet_t *)packet);
		csp_rdp_protocol("RDP %p: RX Queue exists matching Element, seq %u\n", conn, header->seq_nr);

		/* If the matching packet was found, deliver */
		if (header->seq_nr == seq_nr) {
			csp_rdp_protocol("RDP %p: We have a match\n", conn);
			return true;
		}
	}

	return false;
}

static inline int csp_rdp_rx_queue_add(csp_conn_t * conn, csp_packet_t * packet, uint16_t seq_nr) {

	if (csp_rdp_seq_in_rx_queue(conn, seq_nr))
		return -1;
	csp_rdp_queue_rx_add(conn, packet);
	return 0;
}

static void csp_rdp_flush_eack(csp_conn_t * conn, csp_packet_t * eack_packet) {

	/* Loop through TX queue */
	int i, j, count;
	csp_packet_t * packet;
	count = csp_rdp_queue_tx_size();
	for (i = 0; i < count; i++) {

		packet = csp_rdp_queue_tx_get(conn);
		if (packet == NULL) {
			break;
		}

		rdp_header_t * header = csp_rdp_header_ref((csp_packet_t *)packet);
		csp_rdp_protocol("RDP %p: EACK compare element, time %" PRIu32 ", seq %u\n", conn, packet->timestamp_tx, be16toh(header->seq_nr));

		/* Look for this element in EACKs */
		int match = 0;
		for (j = 0; j < (int)((eack_packet->length - sizeof(rdp_header_t)) / sizeof(uint16_t)); j++) {
			if (be16toh(eack_packet->data16[j]) == be16toh(header->seq_nr))
				match = 1;

			/* Enable this if you want EACK's to trigger retransmission */
			if (be16toh(eack_packet->data16[j]) > be16toh(header->seq_nr)) {
				uint32_t time_now = csp_get_ms();
				if (csp_rdp_time_after(time_now, packet->rdp_quarantine)) {
					packet->timestamp_tx = time_now - conn->rdp.packet_timeout - 1;
					packet->rdp_quarantine = time_now + conn->rdp.packet_timeout / 2;
				}
			}
		}

		if (match == 0) {
			/* If not found, put back on tx queue */
			csp_rdp_queue_tx_add(conn, packet);
		} else {
			/* Found, free */
			csp_rdp_protocol("RDP %p: TX Element %u freed\n", conn, be16toh(header->seq_nr));
			csp_buffer_free(packet);
		}
	}
}

static inline bool csp_rdp_should_ack(csp_conn_t * conn) {

	/* If delayed ACKs are not used, always ACK */
	if (!conn->rdp.delayed_acks) {
		return true;
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

int csp_rdp_check_ack(csp_conn_t * conn) {

	/* Check RX queue for spare capacity */
	if (CSP_CONN_RXQUEUE_LEN - csp_queue_size(conn->rx_queue) <= 2 * (int32_t)conn->rdp.window_size) {
		return CSP_ERR_NONE;
	}

	/* If more space available, only send after ack timeout or immediately if delay_acks is zero */
	if (csp_rdp_should_ack(conn)) {
		csp_rdp_send_cmp(conn, NULL, RDP_ACK, conn->rdp.snd_nxt, conn->rdp.rcv_cur);
	}

	return CSP_ERR_NONE;
}

static inline bool csp_rdp_is_conn_ready_for_tx(csp_conn_t * conn) {
	// Check Tx window (messages waiting for acks)
	if (csp_rdp_seq_after(conn->rdp.snd_nxt, conn->rdp.snd_una + conn->rdp.window_size - 1)) {
		return false;
	}
	return true;
}

/**
 * This function must be called with regular intervals for the
 * RDP protocol to work as expected. This takes care of closing
 * stale connections and retransmitting traffic. A good place to
 * call this function is from the CSP router task.
 */
void csp_rdp_check_timeouts(csp_conn_t * conn) {

	const uint32_t time_now = csp_get_ms();

	/**
	 * CONNECTION TIMEOUT:
	 * Check that connection has not timed out inside the network stack
	 */
	if (conn->dest_socket != NULL) {
		if (csp_rdp_time_after(time_now, conn->timestamp + conn->rdp.conn_timeout)) {
			csp_rdp_error("RDP %p: Found a lost connection (now: %" PRIu32 ", ts: %" PRIu32 ", to: %" PRIu32 "), closing\n",
						 conn, time_now, conn->timestamp, conn->rdp.conn_timeout);
			csp_conn_close(conn, CSP_RDP_CLOSED_BY_USERSPACE | CSP_RDP_CLOSED_BY_PROTOCOL | CSP_RDP_CLOSED_BY_TIMEOUT);
			return;
		}
	}

	/**
	 * CLOSE-WAIT TIMEOUT:
	 * After waiting a while in CLOSE-WAIT, the connection should be closed.
	 */
	if (conn->rdp.state == RDP_CLOSE_WAIT) {
		if (csp_rdp_time_after(time_now, conn->timestamp + conn->rdp.conn_timeout)) {
			csp_conn_close(conn, CSP_RDP_CLOSED_BY_PROTOCOL | CSP_RDP_CLOSED_BY_TIMEOUT);
		}
		return;
	}

	/**
	 * MESSAGE TIMEOUT:
	 * Check each outgoing message for TX timeout
	 */
	int count = csp_rdp_queue_tx_size();
	for (int i = 0; i < count; i++) {

		csp_packet_t * packet;
		packet = csp_rdp_queue_tx_get(conn);
		if (packet == NULL) {
			break;
		}

		/* Get header */
		rdp_header_t * header = csp_rdp_header_ref((csp_packet_t *)packet);

		/* If acked, do not retransmit */
		if (csp_rdp_seq_before(be16toh(header->seq_nr), conn->rdp.snd_una)) {
			csp_rdp_protocol("RDP %p: TX Element Free, time %" PRIu32 ", seq %u, una %u\n", conn, packet->timestamp_tx, be16toh(header->seq_nr), conn->rdp.snd_una);
			csp_buffer_free(packet);
			continue;
		}

		/* Check timestamp and retransmit if needed */
		if (csp_rdp_time_after(time_now, packet->timestamp_tx + conn->rdp.packet_timeout)) {
			csp_rdp_protocol("RDP %p: TX Element timed out, retransmitting seq %u\n", conn, be16toh(header->seq_nr));

			/* Update to latest outgoing ACK */
			header->ack_nr = htobe16(conn->rdp.rcv_cur);

			/* Send copy to tx_queue */
			packet->timestamp_tx = csp_get_ms();
			csp_packet_t * new_packet = csp_buffer_clone(packet);
			csp_send_direct(&conn->idout, new_packet, NULL);
		}

		/* Requeue the TX element */
		csp_rdp_queue_tx_add(conn, packet);

	}

	if (conn->rdp.state == RDP_OPEN) {

		/* Check if we have unacknowledged segments */
		if (conn->rdp.delayed_acks) {
			csp_rdp_check_ack(conn);
		}

		/* Wake user task if additional Tx can be done */
		if (csp_rdp_is_conn_ready_for_tx(conn)) {
			//csp_rdp_protocol("RDP %p: Wake Tx task (check timeouts)\n", conn);
			csp_bin_sem_post(&conn->rdp.tx_wait);
		}
	}

	csp_rdp_rx_queue_flush(conn);

}

bool csp_rdp_new_packet(csp_conn_t * conn, csp_packet_t * packet) {

	bool close_connection = false;

	/* Get RX header and convert to host byte-order */
	rdp_header_t * rx_header = csp_rdp_header_ref(packet);
	rx_header->ack_nr = be16toh(rx_header->ack_nr);
	rx_header->seq_nr = be16toh(rx_header->seq_nr);

	uint8_t closed_by = CSP_RDP_CLOSED_BY_PROTOCOL;

	csp_rdp_protocol(
		"RDP %p: Received in S %u: syn %u, ack %u, eack %u, "
		"rst %u, seq_nr %5u, ack_nr %5u, packet_len %u (%u)\n",
		conn, conn->rdp.state, ((rx_header->flags & RDP_SYN) != 0), ((rx_header->flags & RDP_ACK) != 0), ((rx_header->flags & RDP_EAK) != 0),
		((rx_header->flags & RDP_RST) != 0), rx_header->seq_nr, rx_header->ack_nr,
		packet->length, (unsigned int)(packet->length - sizeof(rdp_header_t)));

	/* If a RESET was received. */
	if (rx_header->flags & RDP_RST) {

		if (rx_header->flags & RDP_ACK) {
			/* Store current ack'ed sequence number */
			conn->rdp.snd_una = rx_header->ack_nr + 1;
		}

		if (conn->rdp.state == RDP_CLOSED) {
			csp_rdp_protocol("RDP %p: RST received in CLOSED - ignored\n", conn);
			close_connection = (conn->dest_socket != NULL);
			goto discard_open;
		}

		if (conn->rdp.state == RDP_CLOSE_WAIT) {
			csp_rdp_protocol("RDP %p: RST received in CLOSE_WAIT, ack: %d - closing\n", conn, (rx_header->flags & RDP_ACK));
			if ((rx_header->flags & RDP_ACK) && CSP_USE_RDP_FAST_CLOSE) {
				// skip timeout - the other end has acknowledged the RST
				closed_by |= CSP_RDP_CLOSED_BY_TIMEOUT;
			}
			goto discard_close;
		}

		if (rx_header->seq_nr == (conn->rdp.rcv_cur + 1)) {
			csp_rdp_protocol("RDP %p: Received RST in sequence, no more data incoming, reply with RST\n", conn);
			conn->rdp.state = RDP_CLOSE_WAIT;
			conn->timestamp = csp_get_ms();
			csp_rdp_send_cmp(conn, NULL, RDP_ACK | RDP_RST, conn->rdp.snd_nxt, conn->rdp.rcv_cur);
			if (CSP_USE_RDP_FAST_CLOSE) {
				closed_by |= CSP_RDP_CLOSED_BY_TIMEOUT;
			}
			goto discard_close;
		}

		csp_rdp_protocol("RDP %p: RST out of sequence, keep connection open\n", conn);
		goto discard_open;
	}

	/* The BIG FAT switch (state-machine) */
	switch (conn->rdp.state) {

		/**
		 * STATE == CLOSED
		 */
		case RDP_CLOSED: {

			/* No SYN flag set while in closed. Inform by sending back RST */
			if (!(rx_header->flags & RDP_SYN)) {
				csp_rdp_protocol("RDP %p: Not SYN received in CLOSED state. Discarding packet\n", conn);
				csp_rdp_send_cmp(conn, NULL, RDP_RST, conn->rdp.snd_nxt, conn->rdp.rcv_cur);
				goto discard_close;
			}

			csp_rdp_protocol("RDP %p: SYN-Received\n", conn);

			/* Setup TX seq. */
			unsigned int seed = csp_get_ms();
			conn->rdp.snd_iss = (uint16_t)rand_r(&seed);
			conn->rdp.snd_nxt = conn->rdp.snd_iss + 1;
			conn->rdp.snd_una = conn->rdp.snd_iss;

			/* Store RX seq. */
			conn->rdp.rcv_cur = rx_header->seq_nr;
			conn->rdp.rcv_irs = rx_header->seq_nr;
			conn->rdp.rcv_lsa = rx_header->seq_nr;

			/* Store RDP options */
			conn->rdp.window_size = be32toh(packet->data32[0]);
			conn->rdp.conn_timeout = be32toh(packet->data32[1]);
			conn->rdp.packet_timeout = be32toh(packet->data32[2]);
			conn->rdp.delayed_acks = be32toh(packet->data32[3]);
			conn->rdp.ack_timeout = be32toh(packet->data32[4]);
			conn->rdp.ack_delay_count = be32toh(packet->data32[5]);
			csp_rdp_protocol("RDP %p: window size %" PRIu32 ", conn timeout %" PRIu32 ", packet timeout %" PRIu32 ", delayed acks: %" PRIu32 ", ack timeout %" PRIu32 ", ack each %" PRIu32 " packet\n",
							 conn, conn->rdp.window_size, conn->rdp.conn_timeout, conn->rdp.packet_timeout,
							 conn->rdp.delayed_acks, conn->rdp.ack_timeout, conn->rdp.ack_delay_count);

			/* Connection accepted */
			conn->rdp.state = RDP_SYN_RCVD;

			/* Send SYN/ACK */
			csp_rdp_send_cmp(conn, NULL, RDP_ACK | RDP_SYN, conn->rdp.snd_iss, conn->rdp.rcv_irs);

			goto discard_open;

		} break;

		/**
		 * STATE == SYN-SENT
		 */
		case RDP_SYN_SENT: {

			/* First check SYN/ACK */
			if ((rx_header->flags & RDP_SYN) && (rx_header->flags & RDP_ACK)) {

				conn->rdp.rcv_cur = rx_header->seq_nr;
				conn->rdp.rcv_irs = rx_header->seq_nr;
				conn->rdp.rcv_lsa = rx_header->seq_nr - 1;
				conn->rdp.snd_una = rx_header->ack_nr + 1;
				conn->rdp.ack_timestamp = csp_get_ms();
				conn->rdp.state = RDP_OPEN;

				csp_rdp_protocol("RDP %p: NP: Connection OPEN\n", conn);

				/* Send ACK */
				csp_rdp_send_cmp(conn, NULL, RDP_ACK, conn->rdp.snd_nxt, conn->rdp.rcv_cur);

				/* Wake TX task */
				csp_rdp_protocol("RDP %p: Wake Tx task (ack)\n", conn);
				csp_bin_sem_post(&conn->rdp.tx_wait);

				goto discard_open;
			}

			/* If there was no SYN in the reply, our SYN message hit an already open connection
			 * This is handled by sending a RST.
			 * Normally this would be followed up by a new connection attempt, however
			 * we don't have a method for signaling this to the user space.
			 */
			if ((rx_header->flags & RDP_ACK)) {
				csp_rdp_error("RDP %p: Half-open connection found, send RST and wake Tx task\n", conn);
				csp_rdp_send_cmp(conn, NULL, RDP_RST, conn->rdp.snd_nxt, conn->rdp.rcv_cur);
				csp_bin_sem_post(&conn->rdp.tx_wait);

				goto discard_open;
			}

			/* Otherwise we have an invalid command, such as a SYN reply to a SYN command,
			 * indicating simultaneous connections, which is not possible in the way CSP
			 * reserves some ports for server and some for clients.
			 */
			csp_rdp_error("RDP %p: Invalid reply to SYN request\n", conn);
			goto discard_close;

		} break;

		/**
		 * STATE == OPEN
		 */
		case RDP_SYN_RCVD:
		case RDP_OPEN: {

			/* SYN or !ACK is invalid */
			if ((rx_header->flags & RDP_SYN) || !(rx_header->flags & RDP_ACK)) {
				if (rx_header->seq_nr != conn->rdp.rcv_irs) {
					csp_rdp_error("RDP %p: Invalid SYN or no ACK, resetting!\n", conn);
					goto discard_close;
				} else {
					csp_rdp_protocol("RDP %p: Ignoring duplicate SYN packet!\n", conn);
					goto discard_open;
				}
			}

			/* Check sequence number */
			if (!csp_rdp_seq_between(rx_header->seq_nr, conn->rdp.rcv_cur + 1, conn->rdp.rcv_cur + (conn->rdp.window_size * 2))) {
				csp_rdp_protocol("RDP %p: Invalid sequence number! %u not between %u and %" PRIu32"\n",
								 conn, rx_header->seq_nr, conn->rdp.rcv_cur + 1U, conn->rdp.rcv_cur + (conn->rdp.window_size * 2U));
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
				csp_rdp_error("RDP %p: Invalid ACK number! %u not between %" PRIu32 " and %u\n",
							  conn, rx_header->ack_nr, conn->rdp.snd_una - 1 - (conn->rdp.window_size * 2), conn->rdp.snd_nxt - 1);
				goto discard_open;
			}

			/* Check SYN_RCVD ACK */
			if (conn->rdp.state == RDP_SYN_RCVD) {
				if (rx_header->ack_nr != conn->rdp.snd_iss) {
					csp_rdp_error("RDP %p: SYN-RCVD: Wrong ACK number\n", conn);
					goto discard_close;
				}
				csp_rdp_protocol("RDP %p: NC: Connection OPEN\n", conn);
				conn->rdp.state = RDP_OPEN;

				/* If a socket is set, this message is the first in a new connection
				 * so the connection must be queued to the socket. */
				if (conn->dest_socket != NULL) {

					/* Try queueing */
					if (csp_queue_enqueue(conn->dest_socket->rx_queue, &conn, 0) == CSP_QUEUE_ERROR) {
						csp_rdp_error("RDP %p: ERROR socket cannot accept more connections\n", conn);
						goto discard_close;
					}

					/* Ensure that this connection will not be posted to this socket again
					 * and remember that the connection handle has been passed to userspace
					 * by setting the socket = NULL */
					conn->dest_socket = NULL;
				}
			}

			/* Store current ack'ed sequence number */
			conn->rdp.snd_una = rx_header->ack_nr + 1;

			/* We have an EACK */
			if ((rx_header->flags & RDP_EAK)) {
				if (packet->length > sizeof(rdp_header_t))
					csp_rdp_flush_eack(conn, packet);
				goto discard_open;
			}

			/* If no data, return here */
			if (packet->length <= sizeof(rdp_header_t))
				goto discard_open;

			/* If message is not in sequence, send EACK and store packet */
			if (rx_header->seq_nr != (uint16_t)(conn->rdp.rcv_cur + 1)) {
				if (csp_rdp_rx_queue_add(conn, packet, rx_header->seq_nr) != 0) {
					csp_rdp_protocol("RDP %p: Duplicate sequence number\n", conn);
					csp_rdp_check_ack(conn);
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

		} break;

		case RDP_CLOSE_WAIT:

			/* Ignore SYN or !ACK */
			if ((rx_header->flags & RDP_SYN) || !(rx_header->flags & RDP_ACK)) {
				csp_rdp_protocol("RDP %p: Invalid SYN or no ACK in CLOSE-WAIT\n", conn);
				goto discard_open;
			}

			/* Check ACK number */
			if (!csp_rdp_seq_between(rx_header->ack_nr, conn->rdp.snd_una - 1 - (conn->rdp.window_size * 2), conn->rdp.snd_nxt - 1)) {
				csp_rdp_error("RDP %p: Invalid ACK number! %u not between %" PRIu32 " and %u\n",
							  conn, rx_header->ack_nr, conn->rdp.snd_una - 1 - (conn->rdp.window_size * 2), conn->rdp.snd_nxt - 1);
				goto discard_open;
			}

			/* Store current ack'ed sequence number */
			conn->rdp.snd_una = rx_header->ack_nr + 1;

			/* Send back a reset */
			csp_rdp_send_cmp(conn, NULL, RDP_ACK | RDP_RST, conn->rdp.snd_nxt, conn->rdp.rcv_cur);

			goto discard_open;

		default:
			csp_rdp_error("RDP %p: ERROR default state!\n", conn);
			goto discard_close;
	}

discard_close:
	/* If user-space has received the connection handle, wake it up,
	 * by sending a NULL pointer, user-space must close connection */
	if (conn->dest_socket == NULL) {
		csp_conn_close(conn, closed_by);
		csp_conn_enqueue_packet(conn, NULL);
	} else {
		/* New connection, userspace doesn't know anything about it yet - so it can be completely closed */
		csp_conn_close(conn, closed_by | CSP_RDP_CLOSED_BY_USERSPACE);
	}

discard_open:
	csp_buffer_free(packet);
accepted_open:
	return close_connection;
}

int csp_rdp_connect(csp_conn_t * conn) {

	int retry = 1;

	conn->rdp.window_size = csp_rdp_window_size;
	conn->rdp.conn_timeout = csp_rdp_conn_timeout;
	conn->rdp.packet_timeout = csp_rdp_packet_timeout;
	conn->rdp.delayed_acks = csp_rdp_delayed_acks;
	conn->rdp.ack_timeout = csp_rdp_ack_timeout;
	conn->rdp.ack_delay_count = csp_rdp_ack_delay_count;
	conn->rdp.ack_timestamp = csp_get_ms();

retry:
	csp_rdp_protocol("RDP %p: Active connect, conn state %u\n", conn, conn->rdp.state);

	if (conn->rdp.state == RDP_OPEN) {
		csp_rdp_error("RDP %p: Connection already open\n", conn);
		return CSP_ERR_ALREADY;
	}

	/* Randomize ISS */
	unsigned int seed = csp_get_ms();
	conn->rdp.snd_iss = (uint16_t)rand_r(&seed);
	conn->rdp.snd_nxt = conn->rdp.snd_iss + 1;
	conn->rdp.snd_una = conn->rdp.snd_iss;

	csp_rdp_protocol("RDP %p: AC: Sending SYN\n", conn);

	/* Ensure semaphore is busy, so router task can release it */
	csp_bin_sem_wait(&conn->rdp.tx_wait, 0);

	/* Send SYN message */
	conn->rdp.state = RDP_SYN_SENT;
	if (csp_rdp_send_syn(conn) != CSP_ERR_NONE)
		goto error;

	/* Wait for router task to release semaphore */
	csp_rdp_protocol("RDP %p: AC: Waiting for SYN/ACK reply...\n", conn);
	int result = csp_bin_sem_wait(&conn->rdp.tx_wait, conn->rdp.conn_timeout);

	if (result == CSP_SEMAPHORE_OK) {
		if (conn->rdp.state == RDP_OPEN) {
			csp_rdp_protocol("RDP %p: AC: Connection OPEN\n", conn);
			return CSP_ERR_NONE;
		}
		if (conn->rdp.state == RDP_SYN_SENT) {
			if (retry) {
				csp_rdp_error("RDP %p: Half-open connection detected, RST sent, now retrying\n", conn);
				csp_rdp_queue_flush(conn);
				retry = 0;
				goto retry;
			}
			csp_rdp_error("RDP %p: Connection stayed half-open, even after RST and retry!\n", conn);
			goto error;
		}
	}

error:
	csp_rdp_protocol("RDP %p: AC: Connection Faile\n", conn);
	csp_rdp_close_internal(conn, CSP_RDP_CLOSED_BY_PROTOCOL, false);
	return CSP_ERR_TIMEDOUT;
}

int csp_rdp_send(csp_conn_t * conn, csp_packet_t * packet) {

	if (conn->rdp.state != RDP_OPEN) {
		csp_rdp_error("RDP %p: ERROR cannot send, connection not open (%d)\n", conn, conn->rdp.state);
		return CSP_ERR_RESET;
	}

	while ((conn->rdp.state == RDP_OPEN) && (csp_rdp_is_conn_ready_for_tx(conn) == false)) {
		csp_rdp_protocol("RDP %p: Waiting for window update before sending seq %u\n", conn, conn->rdp.snd_nxt);
		if ((csp_bin_sem_wait(&conn->rdp.tx_wait, conn->rdp.conn_timeout)) != CSP_SEMAPHORE_OK) {
			csp_rdp_error("RDP %p: Timeout during send", conn);
			return CSP_ERR_TIMEDOUT;
		}
	}

	if (conn->rdp.state != RDP_OPEN) {
		csp_rdp_error("RDP %p: ERROR cannot send, connection not open (%d) -> reset\n", conn, conn->rdp.state);
		return CSP_ERR_RESET;
	}

	/* Add RDP header */
	rdp_header_t * tx_header = csp_rdp_header_add(packet);
	if (tx_header == NULL) {
		csp_rdp_error("RDP %p: No space for RDP header (send)\n", conn);
		return CSP_ERR_NOMEM;
	}
	tx_header->ack_nr = htobe16(conn->rdp.rcv_cur);
	tx_header->seq_nr = htobe16(conn->rdp.snd_nxt);
	tx_header->flags |= RDP_ACK;

	/* Send copy to tx_queue */
	csp_packet_t * rdp_packet = csp_buffer_clone(packet);
	if (rdp_packet == NULL) {
		csp_rdp_error("RDP %p: Failed to allocate packet buffer\n", conn);
		return CSP_ERR_NOMEM;
	}

	rdp_packet->timestamp_tx = csp_get_ms();
	rdp_packet->rdp_quarantine = 0;
	csp_rdp_queue_tx_add(conn, rdp_packet);

	csp_rdp_protocol(
		"RDP %p: Sending  in S %u: syn %u, ack %u, eack %u, "
		"rst %u, seq_nr %5u, ack_nr %5u, packet_len %u (%u)\n",
		conn, conn->rdp.state, (tx_header->flags & RDP_SYN), (tx_header->flags & RDP_ACK), (tx_header->flags & RDP_EAK),
		(tx_header->flags & RDP_RST), be16toh(tx_header->seq_nr), be16toh(tx_header->ack_nr),
		packet->length, (unsigned int)(packet->length - sizeof(rdp_header_t)));

	conn->rdp.snd_nxt++;
	return CSP_ERR_NONE;
}

void csp_rdp_init(csp_conn_t * conn) {

	/* Set initial state */
	conn->rdp.state = RDP_CLOSED;
	conn->rdp.closed_by = 0;
	conn->rdp.conn_timeout = csp_rdp_conn_timeout;
	conn->rdp.packet_timeout = csp_rdp_packet_timeout;

	/* Create a binary semaphore to wait on for tasks */
	csp_bin_sem_init(&conn->rdp.tx_wait);

}

/**
 * @note This function may only be called from csp_close, and is therefore
 * without any checks for null pointers.
 */
int csp_rdp_close(csp_conn_t * conn, uint8_t closed_by) {
	return csp_rdp_close_internal(conn, closed_by, true);
}

static int csp_rdp_close_internal(csp_conn_t * conn, uint8_t closed_by, bool send_rst) {

	if (conn->rdp.state == RDP_CLOSED) {
		return CSP_ERR_NONE;
	}

	conn->rdp.closed_by |= closed_by;

	/* If connection is open, send reset */
	if (conn->rdp.state != RDP_CLOSE_WAIT) {
		conn->rdp.state = RDP_CLOSE_WAIT;
		conn->timestamp = csp_get_ms();
		if (send_rst) {
			csp_rdp_send_cmp(conn, NULL, RDP_ACK | RDP_RST, conn->rdp.snd_nxt, conn->rdp.rcv_cur);
		}
		csp_rdp_protocol("RDP %p: csp_rdp_close(0x%x)%s -> CLOSE_WAIT\n", conn, closed_by, send_rst ? ", sent RST" : "");
		csp_bin_sem_post(&conn->rdp.tx_wait);  // wake up any pendng Tx
	}

	if (conn->rdp.closed_by != CSP_RDP_CLOSED_BY_ALL) {
		csp_rdp_protocol("RDP %p: csp_rdp_close(0x%x) != %x, waiting for:%s%s%s\n",
						 conn, closed_by, conn->rdp.closed_by,
						 (conn->rdp.closed_by & CSP_RDP_CLOSED_BY_USERSPACE) ? "" : " userspace",
						 (conn->rdp.closed_by & CSP_RDP_CLOSED_BY_PROTOCOL) ? "" : " protocol",
						 (conn->rdp.closed_by & CSP_RDP_CLOSED_BY_TIMEOUT) ? "" : " timeout");
		return CSP_ERR_AGAIN;
	}

	csp_rdp_protocol("RDP %p: csp_rdp_close(0x%x) -> CLOSED\n", conn, closed_by);
	conn->rdp.state = RDP_CLOSED;
	conn->rdp.closed_by = 0;
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


#endif  // CSP_USE_RDP
