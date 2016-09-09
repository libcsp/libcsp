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

/* CAN frames contains at most 8 bytes of data, so in order to transmit CSP
 * packets larger than this, a fragmentation protocol is required. The CAN
 * Fragmentation Protocol (CFP) header is designed to match the 29 bit CAN
 * identifier.
 *
 * The CAN identifier is divided in these fields:
 * src:          5 bits
 * dst:          5 bits
 * type:         1 bit
 * remain:       8 bits
 * identifier:   10 bits
 *
 * Source and Destination addresses must match the CSP packet. The type field
 * is used to distinguish the first and subsequent frames in a fragmented CSP
 * packet. Type is BEGIN (0) for the first fragment and MORE (1) for all other
 * fragments. Remain indicates number of remaining fragments, and must be
 * decremented by one for each fragment sent. The identifier field serves the
 * same purpose as in the Internet Protocol, and should be an auto incrementing
 * integer to uniquely separate sessions.
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include <csp/csp.h>
#include <csp/csp_interface.h>
#include <csp/csp_endian.h>
#include <csp/interfaces/csp_if_can.h>

#include <csp/arch/csp_semaphore.h>
#include <csp/arch/csp_time.h>
#include <csp/arch/csp_queue.h>
#include <csp/arch/csp_thread.h>

#include <csp/drivers/can.h>

/* CAN header macros */
#define CFP_HOST_SIZE		5
#define CFP_TYPE_SIZE		1
#define CFP_REMAIN_SIZE		8
#define CFP_ID_SIZE		10

/* Macros for extracting header fields */
#define CFP_FIELD(id,rsiz,fsiz) ((uint32_t)((uint32_t)((id) >> (rsiz)) & (uint32_t)((1 << (fsiz)) - 1)))
#define CFP_SRC(id)		CFP_FIELD(id, CFP_HOST_SIZE + CFP_TYPE_SIZE + CFP_REMAIN_SIZE + CFP_ID_SIZE, CFP_HOST_SIZE)
#define CFP_DST(id)		CFP_FIELD(id, CFP_TYPE_SIZE + CFP_REMAIN_SIZE + CFP_ID_SIZE, CFP_HOST_SIZE)
#define CFP_TYPE(id)		CFP_FIELD(id, CFP_REMAIN_SIZE + CFP_ID_SIZE, CFP_TYPE_SIZE)
#define CFP_REMAIN(id)		CFP_FIELD(id, CFP_ID_SIZE, CFP_REMAIN_SIZE)
#define CFP_ID(id)		CFP_FIELD(id, 0, CFP_ID_SIZE)

/* Macros for building CFP headers */
#define CFP_MAKE_FIELD(id,fsiz,rsiz) ((uint32_t)(((id) & (uint32_t)((uint32_t)(1 << (fsiz)) - 1)) << (rsiz)))
#define CFP_MAKE_SRC(id)	CFP_MAKE_FIELD(id, CFP_HOST_SIZE, CFP_HOST_SIZE + CFP_TYPE_SIZE + CFP_REMAIN_SIZE + CFP_ID_SIZE)
#define CFP_MAKE_DST(id)	CFP_MAKE_FIELD(id, CFP_HOST_SIZE, CFP_TYPE_SIZE + CFP_REMAIN_SIZE + CFP_ID_SIZE)
#define CFP_MAKE_TYPE(id)	CFP_MAKE_FIELD(id, CFP_TYPE_SIZE, CFP_REMAIN_SIZE + CFP_ID_SIZE)
#define CFP_MAKE_REMAIN(id)	CFP_MAKE_FIELD(id, CFP_REMAIN_SIZE, CFP_ID_SIZE)
#define CFP_MAKE_ID(id)		CFP_MAKE_FIELD(id, CFP_ID_SIZE, 0)

/* Mask to uniquely separate connections */
#define CFP_ID_CONN_MASK	(CFP_MAKE_SRC((uint32_t)(1 << CFP_HOST_SIZE) - 1) | \
				 CFP_MAKE_DST((uint32_t)(1 << CFP_HOST_SIZE) - 1) | \
				 CFP_MAKE_ID((uint32_t)(1 << CFP_ID_SIZE) - 1))

/* Maximum Transmission Unit for CSP over CAN */
#define CSP_CAN_MTU		256

/* Maximum number of frames in RX queue */
#define CSP_CAN_RX_QUEUE_SIZE	100

/* Number of packet buffer elements */
#define PBUF_ELEMENTS		CSP_CONN_MAX

/* Buffer element timeout in ms */
#define PBUF_TIMEOUT_MS		10000

/* CFP Frame Types */
enum cfp_frame_t {
	CFP_BEGIN = 0,
	CFP_MORE = 1
};

/* CFP identification number */
static int csp_can_id = 0;

/* CFP identification number semaphore */
static csp_bin_sem_handle_t csp_can_id_sem;

/* RX task handle */
static csp_thread_handle_t csp_can_rx_task_h;

/* RX frame queue */
static csp_queue_handle_t csp_can_rx_queue;

/* Identification number */
static int csp_can_id_init(void)
{
	/* Init ID field to random number */
	srand((int)csp_get_ms());
	csp_can_id = rand() & ((1 << CFP_ID_SIZE) - 1);

	if (csp_bin_sem_create(&csp_can_id_sem) == CSP_SEMAPHORE_OK) {
		return CSP_ERR_NONE;
	} else {
		csp_log_error("Could not initialize CFP id semaphore");
		return CSP_ERR_NOMEM;
	}
}

static int csp_can_id_get(void)
{
	int id;
	if (csp_bin_sem_wait(&csp_can_id_sem, 1000) != CSP_SEMAPHORE_OK)
		return CSP_ERR_TIMEDOUT;
	id = csp_can_id++;
	csp_can_id = csp_can_id & ((1 << CFP_ID_SIZE) - 1);
	csp_bin_sem_post(&csp_can_id_sem);
	return id;
}

/* Packet buffers */
typedef enum {
	BUF_FREE = 0,			/* Buffer element free */
	BUF_USED = 1,			/* Buffer element used */
} csp_can_pbuf_state_t;

typedef struct {
	uint16_t rx_count;		/* Received bytes */
	uint32_t remain;		/* Remaining packets */
	uint32_t cfpid;			/* Connection CFP identification number */
	csp_packet_t *packet;		/* Pointer to packet buffer */
	csp_can_pbuf_state_t state;	/* Element state */
	uint32_t last_used;		/* Timestamp in ms for last use of buffer */
} csp_can_pbuf_element_t;

static csp_can_pbuf_element_t csp_can_pbuf[PBUF_ELEMENTS];

static int csp_can_pbuf_init(void)
{
	/* Initialize packet buffers */
	int i;
	csp_can_pbuf_element_t *buf;

	for (i = 0; i < PBUF_ELEMENTS; i++) {
		buf = &csp_can_pbuf[i];
		buf->rx_count = 0;
		buf->cfpid = 0;
		buf->packet = NULL;
		buf->state = BUF_FREE;
		buf->last_used = 0;
		buf->remain = 0;
	}

	return CSP_ERR_NONE;
}

static void csp_can_pbuf_timestamp(csp_can_pbuf_element_t *buf)
{
	buf->last_used = csp_get_ms();
}

static int csp_can_pbuf_free(csp_can_pbuf_element_t *buf)
{
	/* Free CSP packet */
	if (buf->packet != NULL)
		csp_buffer_free(buf->packet);

	/* Mark buffer element free */
	buf->packet = NULL;
	buf->state = BUF_FREE;
	buf->rx_count = 0;
	buf->cfpid = 0;
	buf->last_used = 0;
	buf->remain = 0;

	return CSP_ERR_NONE;
}

static csp_can_pbuf_element_t *csp_can_pbuf_new(uint32_t id)
{
	int i;
	csp_can_pbuf_element_t *buf, *ret = NULL;

	for (i = 0; i < PBUF_ELEMENTS; i++) {
		buf = &csp_can_pbuf[i];
		if (buf->state == BUF_FREE) {
			buf->state = BUF_USED;
			buf->cfpid = id;
			buf->remain = 0;
			csp_can_pbuf_timestamp(buf);
			ret = buf;
			break;
		}
	}

	return ret;
}

static csp_can_pbuf_element_t *csp_can_pbuf_find(uint32_t id, uint32_t mask)
{
	int i;
	csp_can_pbuf_element_t *buf, *ret = NULL;

	for (i = 0; i < PBUF_ELEMENTS; i++) {
		buf = &csp_can_pbuf[i];

		if ((buf->state == BUF_USED) && ((buf->cfpid & mask) == (id & mask))) {
			csp_can_pbuf_timestamp(buf);
			ret = buf;
			break;
		}
	}

	return ret;
}

static void csp_can_pbuf_cleanup(void)
{
	int i;
	csp_can_pbuf_element_t *buf;

	for (i = 0; i < PBUF_ELEMENTS; i++) {
		buf = &csp_can_pbuf[i];

		/* Skip if not used */
		if (buf->state != BUF_USED)
			continue;

		/* Check timeout */
		uint32_t now = csp_get_ms();
		if (now - buf->last_used > PBUF_TIMEOUT_MS) {
			csp_log_warn("CAN Buffer element timed out");
			/* Recycle packet buffer */
			csp_can_pbuf_free(buf);
		}
	}
}

static int csp_can_process_frame(can_frame_t *frame)
{
	csp_can_pbuf_element_t *buf;
	uint8_t offset;

	can_id_t id = frame->id;

	/* Bind incoming frame to a packet buffer */
	buf = csp_can_pbuf_find(id, CFP_ID_CONN_MASK);

	/* Check returned buffer */
	if (buf == NULL) {
		if (CFP_TYPE(id) == CFP_BEGIN) {
			buf = csp_can_pbuf_new(id);
			if (buf == NULL) {
				csp_log_warn("No available packet buffer for CAN");
				csp_if_can.rx_error++;
				return CSP_ERR_NOMEM;
			}
		} else {
			csp_log_warn("Out of order MORE frame received");
			csp_if_can.frame++;
			return CSP_ERR_INVAL;
		}
	}

	/* Reset frame data offset */
	offset = 0;

	switch (CFP_TYPE(id)) {

	case CFP_BEGIN:

		/* Discard packet if DLC is less than CSP id + CSP length fields */
		if (frame->dlc < sizeof(csp_id_t) + sizeof(uint16_t)) {
			csp_log_warn("Short BEGIN frame received");
			csp_if_can.frame++;
			csp_can_pbuf_free(buf);
			break;
		}

		/* Check for incomplete frame */
		if (buf->packet != NULL) {
			/* Reuse the buffer */
			csp_log_warn("Incomplete frame");
			csp_if_can.frame++;
		} else {
			/* Allocate memory for frame */
			buf->packet = csp_buffer_get(CSP_CAN_MTU);
			if (buf->packet == NULL) {
				csp_log_error("Failed to get buffer for CSP_BEGIN packet");
				csp_if_can.frame++;
				csp_can_pbuf_free(buf);
				break;
			}
		}

		/* Copy CSP identifier and length*/
		memcpy(&(buf->packet->id), frame->data, sizeof(csp_id_t));
		buf->packet->id.ext = csp_ntoh32(buf->packet->id.ext);
		memcpy(&(buf->packet->length), frame->data + sizeof(csp_id_t), sizeof(uint16_t));
		buf->packet->length = csp_ntoh16(buf->packet->length);

		/* Reset RX count */
		buf->rx_count = 0;

		/* Set offset to prevent CSP header from being copied to CSP data */
		offset = sizeof(csp_id_t) + sizeof(uint16_t);

		/* Set remain field - increment to include begin packet */
		buf->remain = CFP_REMAIN(id) + 1;

		/* FALLTHROUGH */

	case CFP_MORE:

		/* Check 'remain' field match */
		if (CFP_REMAIN(id) != buf->remain - 1) {
			csp_log_error("CAN frame lost in CSP packet");
			csp_can_pbuf_free(buf);
			csp_if_can.frame++;
			break;
		}

		/* Decrement remaining frames */
		buf->remain--;

		/* Check for overflow */
		if ((buf->rx_count + frame->dlc - offset) > buf->packet->length) {
			csp_log_error("RX buffer overflow");
			csp_if_can.frame++;
			csp_can_pbuf_free(buf);
			break;
		}

		/* Copy dlc bytes into buffer */
		memcpy(&buf->packet->data[buf->rx_count], frame->data + offset, frame->dlc - offset);
		buf->rx_count += frame->dlc - offset;

		/* Check if more data is expected */
		if (buf->rx_count != buf->packet->length)
			break;

		/* Data is available */
		csp_new_packet(buf->packet, &csp_if_can, NULL);

		/* Drop packet buffer reference */
		buf->packet = NULL;

		/* Free packet buffer */
		csp_can_pbuf_free(buf);

		break;

	default:
		csp_log_warn("Received unknown CFP message type");
		csp_can_pbuf_free(buf);
		break;

	}

	return CSP_ERR_NONE;
}

static CSP_DEFINE_TASK(csp_can_rx_task)
{
	int ret;
	can_frame_t frame;

	while (1) {
		ret = csp_queue_dequeue(csp_can_rx_queue, &frame, 1000);
		if (ret != CSP_QUEUE_OK) {
			csp_can_pbuf_cleanup();
			continue;
		}

		csp_can_process_frame(&frame);
	}

	csp_thread_exit();
}

int csp_can_rx_frame(can_frame_t *frame, CSP_BASE_TYPE *task_woken)
{
	if (csp_queue_enqueue_isr(csp_can_rx_queue, frame, task_woken) != CSP_QUEUE_OK)
		return CSP_ERR_NOMEM;

	return CSP_ERR_NONE;
}

static int csp_can_tx(csp_iface_t *interface, csp_packet_t *packet, uint32_t timeout)
{
	uint16_t tx_count;
	uint8_t bytes, overhead, avail, dest;
	uint8_t frame_buf[8];

	/* Get CFP identification number */
	int ident = csp_can_id_get();
	if (ident < 0) {
		csp_log_warn("Failed to get CFP identification number");
		return CSP_ERR_INVAL;
	}

	/* Calculate overhead */
	overhead = sizeof(csp_id_t) + sizeof(uint16_t);

	/* Insert destination node mac address into the CFP destination field */
	dest = csp_rtable_find_mac(packet->id.dst);
	if (dest == CSP_NODE_MAC)
		dest = packet->id.dst;

	/* Create CAN identifier */
	can_id_t id = 0;
	id |= CFP_MAKE_SRC(packet->id.src);
	id |= CFP_MAKE_DST(dest);
	id |= CFP_MAKE_ID(ident);
	id |= CFP_MAKE_TYPE(CFP_BEGIN);
	id |= CFP_MAKE_REMAIN((packet->length + overhead - 1) / 8);

	/* Calculate first frame data bytes */
	avail = 8 - overhead;
	bytes = (packet->length <= avail) ? packet->length : avail;

	/* Copy CSP headers and data */
	uint32_t csp_id_be = csp_hton32(packet->id.ext);
	uint16_t csp_length_be = csp_hton16(packet->length);

	memcpy(frame_buf, &csp_id_be, sizeof(csp_id_be));
	memcpy(frame_buf + sizeof(csp_id_be), &csp_length_be, sizeof(csp_length_be));
	memcpy(frame_buf + overhead, packet->data, bytes);

	/* Increment tx counter */
	tx_count = bytes;

	/* Send first frame */
	if (can_send(id, frame_buf, overhead + bytes)) {
		csp_log_warn("Failed to send CAN frame in csp_tx_can");
		return CSP_ERR_DRIVER;
	}

	/* Send next frames if not complete */
	while (tx_count < packet->length) {
		/* Calculate frame data bytes */
		bytes = (packet->length - tx_count >= 8) ? 8 : packet->length - tx_count;

		/* Prepare identifier */
		can_id_t id = 0;
		id |= CFP_MAKE_SRC(packet->id.src);
		id |= CFP_MAKE_DST(dest);
		id |= CFP_MAKE_ID(ident);
		id |= CFP_MAKE_TYPE(CFP_MORE);
		id |= CFP_MAKE_REMAIN((packet->length - tx_count - bytes + 7) / 8);

		/* Increment tx counter */
		tx_count += bytes;

		/* Send frame */
		if (can_send(id, packet->data + tx_count - bytes, bytes)) {
			csp_log_warn("Failed to send CAN frame in Tx callback");
			csp_if_can.tx_error++;
			return CSP_ERR_DRIVER;
		}
	}

	csp_buffer_free(packet);

	return CSP_ERR_NONE;
}

int csp_can_init(uint8_t mode, struct csp_can_config *conf)
{
	int ret;
	uint32_t mask;

	/* Initialize packet buffer */
	if (csp_can_pbuf_init() != 0) {
		csp_log_error("Failed to initialize CAN packet buffers");
		return CSP_ERR_NOMEM;
	}

	/* Initialize CFP identifier */
	if (csp_can_id_init() != 0) {
		csp_log_error("Failed to initialize CAN identification number");
		return CSP_ERR_NOMEM;
	}

	if (mode == CSP_CAN_MASKED) {
		mask = CFP_MAKE_DST((1 << CFP_HOST_SIZE) - 1);
	} else if (mode == CSP_CAN_PROMISC) {
		mask = 0;
	} else {
		csp_log_error("Unknown CAN mode");
		return CSP_ERR_INVAL;
	}

	csp_can_rx_queue = csp_queue_create(CSP_CAN_RX_QUEUE_SIZE, sizeof(can_frame_t));
	if (!csp_can_rx_queue) {
		csp_log_error("Failed to create CAN RX queue");
		return CSP_ERR_NOMEM;
	}

	ret = csp_thread_create(csp_can_rx_task, "CAN", 6000/sizeof(int), NULL, 3, &csp_can_rx_task_h);
	if (ret != 0) {
		csp_log_error("Failed to init CAN RX task");
		return CSP_ERR_NOMEM;
	}

	/* Initialize CAN driver */
	if (can_init(CFP_MAKE_DST(csp_get_address()), mask, conf) != 0) {
		csp_log_error("Failed to initialize CAN driver");
		return CSP_ERR_DRIVER;
	}

	/* Regsiter interface */
	csp_iflist_add(&csp_if_can);

	return CSP_ERR_NONE;
}

/** Interface definition */
csp_iface_t csp_if_can = {
	.name = "CAN",
	.nexthop = csp_can_tx,
	.mtu = CSP_CAN_MTU,
};
