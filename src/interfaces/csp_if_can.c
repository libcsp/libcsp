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

#include "csp_if_can_pbuf.h"

/* CFP Frame Types */
enum cfp_frame_t {
	CFP_BEGIN = 0,
	CFP_MORE = 1
};

int csp_can_rx(csp_iface_t *interface, uint32_t id, uint8_t *data, uint8_t dlc, CSP_BASE_TYPE *task_woken)
{
	csp_can_pbuf_element_t *buf;
	uint8_t offset;

	/* Random packet loss */
#if 0
	int random = rand();
	if (random < RAND_MAX * 0.00005) {
		csp_log_warn("Dropping frame");
		return;
	}
#endif

	/* Bind incoming frame to a packet buffer */
	buf = csp_can_pbuf_find(id, CFP_ID_CONN_MASK);

	/* Check returned buffer */
	if (buf == NULL) {
		if (CFP_TYPE(id) == CFP_BEGIN) {
			buf = csp_can_pbuf_new(id, task_woken);
			if (buf == NULL) {
				//csp_log_warn("No available packet buffer for CAN");
				interface->rx_error++;
				return CSP_ERR_NOMEM;
			}
		} else {
			//csp_log_warn("Out of order id 0x%X remain %u", CFP_ID(id), CFP_REMAIN(id));
			interface->frame++;
			return CSP_ERR_INVAL;
		}
	}

	/* Reset frame data offset */
	offset = 0;

	switch (CFP_TYPE(id)) {

	case CFP_BEGIN:

		/* Discard packet if DLC is less than CSP id + CSP length fields */
		if (dlc < sizeof(csp_id_t) + sizeof(uint16_t)) {
			//csp_log_warn("Short BEGIN frame received");
			interface->frame++;
			csp_can_pbuf_free(buf, task_woken);
			break;
		}

		/* Check for incomplete frame */
		if (buf->packet != NULL) {
			/* Reuse the buffer */
			//csp_log_warn("Incomplete frame");
			interface->frame++;
		} else {
			/* Allocate memory for frame */
			if (task_woken == NULL) {
				buf->packet = csp_buffer_get(CSP_CAN_MTU);
			} else {
				buf->packet = csp_buffer_get_isr(CSP_CAN_MTU);
			}
			if (buf->packet == NULL) {
				//csp_log_error("Failed to get buffer for CSP_BEGIN packet");
				interface->frame++;
				csp_can_pbuf_free(buf, task_woken);
				break;
			}
		}

		/* Copy CSP identifier and length*/
		memcpy(&(buf->packet->id), data, sizeof(csp_id_t));
		buf->packet->id.ext = csp_ntoh32(buf->packet->id.ext);
		memcpy(&(buf->packet->length), data + sizeof(csp_id_t), sizeof(uint16_t));
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
			//csp_log_error("CAN frame lost in CSP packet");
			csp_can_pbuf_free(buf, task_woken);
			interface->frame++;
			break;
		}

		/* Decrement remaining frames */
		buf->remain--;

		/* Check for overflow */
		if ((buf->rx_count + dlc - offset) > buf->packet->length) {
			//csp_log_error("RX buffer overflow");
			interface->frame++;
			csp_can_pbuf_free(buf, task_woken);
			break;
		}

		/* Copy dlc bytes into buffer */
		memcpy(&buf->packet->data[buf->rx_count], data + offset, dlc - offset);
		buf->rx_count += dlc - offset;

		/* Check if more data is expected */
		if (buf->rx_count != buf->packet->length)
			break;

		/* Data is available */
		csp_qfifo_write(buf->packet, interface, task_woken);

		/* Drop packet buffer reference */
		buf->packet = NULL;

		/* Free packet buffer */
		csp_can_pbuf_free(buf, task_woken);

		break;

	default:
		//csp_log_warn("Received unknown CFP message type");
		csp_can_pbuf_free(buf, task_woken);
		break;

	}

	return CSP_ERR_NONE;
}

int csp_can_tx(csp_iface_t *interface, csp_packet_t *packet, uint32_t timeout)
{

	/* CFP Identification number */
	static volatile int csp_can_frame_id = 0;

	/* Get local copy of the static frameid */
	int ident = csp_can_frame_id++;

	uint16_t tx_count;
	uint8_t bytes, overhead, avail, dest;
	uint8_t frame_buf[8];

	/* Calculate overhead */
	overhead = sizeof(csp_id_t) + sizeof(uint16_t);

	/* Insert destination node mac address into the CFP destination field */
	dest = csp_rtable_find_mac(packet->id.dst);
	if (dest == CSP_NODE_MAC)
		dest = packet->id.dst;

	/* Create CAN identifier */
	uint32_t id = 0;
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
	if (csp_can_tx_frame(interface, id, frame_buf, overhead + bytes)) {
		//csp_log_warn("Failed to send CAN frame in csp_tx_can");
		interface->tx_error++;
		return CSP_ERR_DRIVER;
	}

	/* Send next frames if not complete */
	while (tx_count < packet->length) {
		/* Calculate frame data bytes */
		bytes = (packet->length - tx_count >= 8) ? 8 : packet->length - tx_count;

		/* Prepare identifier */
		uint32_t id = 0;
		id |= CFP_MAKE_SRC(packet->id.src);
		id |= CFP_MAKE_DST(dest);
		id |= CFP_MAKE_ID(ident);
		id |= CFP_MAKE_TYPE(CFP_MORE);
		id |= CFP_MAKE_REMAIN((packet->length - tx_count - bytes + 7) / 8);

		/* Increment tx counter */
		tx_count += bytes;

		/* Send frame */
		if (csp_can_tx_frame(interface, id, packet->data + tx_count - bytes, bytes)) {
			//csp_log_warn("Failed to send CAN frame in Tx callback");
			interface->tx_error++;
			return CSP_ERR_DRIVER;
		}
	}

	csp_buffer_free(packet);

	return CSP_ERR_NONE;
}
