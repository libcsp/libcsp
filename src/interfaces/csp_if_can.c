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

#include <csp/interfaces/csp_if_can.h>

#include <string.h>
#include <stdlib.h>

#include <csp/csp.h>
#include <csp/csp_endian.h>
#include <csp/arch/csp_semaphore.h>

#include "csp_if_can_pbuf.h"

/* Max number of bytes per CAN frame */
#define MAX_BYTES_IN_CAN_FRAME 8
#define CFP_OVERHEAD           (sizeof(csp_id_t) + sizeof(uint16_t))
#define MAX_CAN_DATA_SIZE      (((1 << CFP_REMAIN_SIZE) * MAX_BYTES_IN_CAN_FRAME) - CFP_OVERHEAD)

/* CFP type */
enum cfp_frame_t {
	/* First CFP fragment of a CSP packet */
	CFP_BEGIN = 0,
	/* Remaining CFP fragment(s) of a CSP packet */
	CFP_MORE = 1
};

int csp_can_rx(csp_iface_t *iface, uint32_t id, const uint8_t *data, uint8_t dlc, CSP_BASE_TYPE *task_woken)
{
	/* Test: random packet loss */
        if (0) {
		int random = rand();
		if (random < RAND_MAX * 0.00005) {
			csp_log_warn("Dropping frame");
			return CSP_ERR_DRIVER;
		}
	}

	/* Bind incoming frame to a packet buffer */
	csp_can_pbuf_element_t * buf = csp_can_pbuf_find(id, CFP_ID_CONN_MASK, task_woken);
	if (buf == NULL) {
		if (CFP_TYPE(id) == CFP_BEGIN) {
			buf = csp_can_pbuf_new(id, task_woken);
			if (buf == NULL) {
				//csp_log_warn("No available packet buffer for CAN");
				iface->rx_error++;
				return CSP_ERR_NOMEM;
			}
		} else {
			//csp_log_warn("Out of order id 0x%X remain %u", CFP_ID(id), CFP_REMAIN(id));
			iface->frame++;
			return CSP_ERR_INVAL;
		}
	}

	/* Reset frame data offset */
	uint8_t offset = 0;

	switch (CFP_TYPE(id)) {

	case CFP_BEGIN:

		/* Discard packet if DLC is less than CSP id + CSP length fields */
		if (dlc < (sizeof(csp_id_t) + sizeof(uint16_t))) {
			//csp_log_warn("Short BEGIN frame received");
			iface->frame++;
			csp_can_pbuf_free(buf, task_woken);
			break;
		}

		/* Check for incomplete frame */
		if (buf->packet != NULL) {
			/* Reuse the buffer */
			//csp_log_warn("Incomplete frame");
			iface->frame++;
		} else {
			/* Get free buffer for frame */
			buf->packet = task_woken ? csp_buffer_get_isr(0) : csp_buffer_get(0); // CSP only supports one size
			if (buf->packet == NULL) {
				//csp_log_error("Failed to get buffer for CSP_BEGIN packet");
				iface->frame++;
				csp_can_pbuf_free(buf, task_woken);
				break;
			}
		}

		/* Copy CSP identifier (header) */
		memcpy(&(buf->packet->id), data, sizeof(buf->packet->id));
		buf->packet->id.ext = csp_ntoh32(buf->packet->id.ext);

		/* Copy CSP length (of data) */
		memcpy(&(buf->packet->length), data + sizeof(csp_id_t), sizeof(buf->packet->length));
		buf->packet->length = csp_ntoh16(buf->packet->length);

		/* Check length against max */
		if ((buf->packet->length > MAX_CAN_DATA_SIZE) || (buf->packet->length > csp_buffer_data_size())) {
			iface->rx_error++;
			csp_can_pbuf_free(buf, task_woken);
			break;
		}

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
			iface->frame++;
			break;
		}

		/* Decrement remaining frames */
		buf->remain--;

		/* Check for overflow */
		if ((buf->rx_count + dlc - offset) > buf->packet->length) {
			//csp_log_error("RX buffer overflow");
			iface->frame++;
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
		csp_qfifo_write(buf->packet, iface, task_woken);

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

int csp_can_tx(const csp_route_t * ifroute, csp_packet_t *packet)
{
        csp_iface_t * iface = ifroute->iface;
        csp_can_interface_data_t * ifdata = iface->interface_data;

	/* Get an unique CFP id - this should be locked to prevent access from multiple tasks */
	const uint32_t ident = ifdata->cfp_frame_id++;

	/* Check protocol's max length - limit is 1 (first) frame + as many frames that can be specified in 'remain' */
        if (packet->length > MAX_CAN_DATA_SIZE) {
		return CSP_ERR_TX;
        }

	/* Insert destination node/via address into the CFP destination field */
	const uint8_t dest = (ifroute->via != CSP_NO_VIA_ADDRESS) ? ifroute->via : packet->id.dst;

	/* Create CAN identifier */
	uint32_t id = (CFP_MAKE_SRC(packet->id.src) |
                       CFP_MAKE_DST(dest) |
                       CFP_MAKE_ID(ident) |
                       CFP_MAKE_TYPE(CFP_BEGIN) |
                       CFP_MAKE_REMAIN((packet->length + CFP_OVERHEAD - 1) / MAX_BYTES_IN_CAN_FRAME));

	/* Calculate first frame data bytes */
	const uint8_t avail = MAX_BYTES_IN_CAN_FRAME - CFP_OVERHEAD;
	uint8_t bytes = (packet->length <= avail) ? packet->length : avail;

	/* Copy CSP headers and data */
	const uint32_t csp_id_be = csp_hton32(packet->id.ext);
	const uint16_t csp_length_be = csp_hton16(packet->length);

	uint8_t frame_buf[MAX_BYTES_IN_CAN_FRAME];
	memcpy(frame_buf, &csp_id_be, sizeof(csp_id_be));
	memcpy(frame_buf + sizeof(csp_id_be), &csp_length_be, sizeof(csp_length_be));
	memcpy(frame_buf + CFP_OVERHEAD, packet->data, bytes);

	/* Increment tx counter */
	uint16_t tx_count = bytes;

        const csp_can_driver_tx_t tx_func = ifdata->tx_func;

	/* Send first frame */
	if ((tx_func)(iface->driver_data, id, frame_buf, CFP_OVERHEAD + bytes) != CSP_ERR_NONE) {
		//csp_log_warn("Failed to send CAN frame in csp_tx_can");
		iface->tx_error++;
		return CSP_ERR_DRIVER;
	}

	/* Send next frames if not complete */
	while (tx_count < packet->length) {
		/* Calculate frame data bytes */
		bytes = (packet->length - tx_count >= MAX_BYTES_IN_CAN_FRAME) ? MAX_BYTES_IN_CAN_FRAME : packet->length - tx_count;

		/* Prepare identifier */
		id = (CFP_MAKE_SRC(packet->id.src) |
                      CFP_MAKE_DST(dest) |
                      CFP_MAKE_ID(ident) |
                      CFP_MAKE_TYPE(CFP_MORE) |
                      CFP_MAKE_REMAIN((packet->length - tx_count - bytes + MAX_BYTES_IN_CAN_FRAME - 1) / MAX_BYTES_IN_CAN_FRAME));

		/* Increment tx counter */
		tx_count += bytes;

		/* Send frame */
		if ((tx_func)(iface->driver_data, id, packet->data + tx_count - bytes, bytes) != CSP_ERR_NONE) {
			//csp_log_warn("Failed to send CAN frame in Tx callback");
			iface->tx_error++;
			return CSP_ERR_DRIVER;
		}
	}

	csp_buffer_free(packet);

	return CSP_ERR_NONE;
}

int csp_can_add_interface(csp_iface_t * iface) {

	if ((iface == NULL) || (iface->name == NULL) || (iface->interface_data == NULL)) {
		return CSP_ERR_INVAL;
	}

        csp_can_interface_data_t * ifdata = iface->interface_data;
	if (ifdata->tx_func == NULL) {
		return CSP_ERR_INVAL;
	}

        if ((iface->mtu == 0) || (iface->mtu > MAX_CAN_DATA_SIZE)) {
            iface->mtu = MAX_CAN_DATA_SIZE;
        }

        ifdata->cfp_frame_id = 0;

	iface->nexthop = csp_can_tx;

	return csp_iflist_add(iface);
}
