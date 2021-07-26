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
#include "../csp_id.h"
#include "../csp_init.h"

/**
 * TESTING:
 *
 * Create a virtual CAN network interface with a specific name 'vcan42':
 *    $ sudo ip link add dev vcan42 type vcan
 *    $ sudo ip link set dev vcan42 down
 *    $ sudo ip link set dev vcan42 up type can
 *
 */

/* Max number of bytes per CAN frame */
#define CAN_FRAME_SIZE 8

/**
 * CFP 1.x defines
 */
#define CFP1_CSP_HEADER_OFFSET 0
#define CFP1_CSP_HEADER_SIZE 4
#define CFP1_DATA_LEN_OFFSET 4
#define CFP1_DATA_LEN_SIZE 2
#define CFP1_DATA_OFFSET 6
#define CFP1_DATA_SIZE_BEGIN 2
#define CFP1_DATA_SIZE_MORE 8

/* CFP type */
enum cfp_frame_t {
	/* First CFP fragment of a CSP packet */
	CFP_BEGIN = 0,
	/* Remaining CFP fragment(s) of a CSP packet */
	CFP_MORE = 1
};

int csp_can1_rx(csp_iface_t *iface, uint32_t id, const uint8_t *data, uint8_t dlc, int *task_woken)
{
	/* Test: random packet loss */
    if (0) {
		int random = rand();
		if (random < RAND_MAX * 0.00005) {
			//csp_log_warn("Dropping frame");
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
		if (dlc < (sizeof(uint32_t) + sizeof(uint16_t))) {
			csp_log_warn("Short BEGIN frame received");
			iface->frame++;
			csp_can_pbuf_free(buf, task_woken);
			break;
		}

		/* Check for incomplete frame */
		if (buf->packet != NULL) {
			/* Reuse the buffer */
			csp_log_warn("Incomplete frame");
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

		csp_id1_setup_rx(buf->packet);

		/* Copy CSP identifier (header) */
		memcpy(buf->packet->frame_begin, data, sizeof(uint32_t));
		buf->packet->frame_length += sizeof(uint32_t);

		csp_id1_strip(buf->packet);

		/* Copy CSP length (of data) */
		memcpy(&(buf->packet->length), data + sizeof(uint32_t), sizeof(buf->packet->length));
		buf->packet->length = csp_ntoh16(buf->packet->length);

		/* Check if frame exceeds MTU */
		if (buf->packet->length > iface->mtu) {
			iface->rx_error++;
			csp_can_pbuf_free(buf, task_woken);
			break;
		}

		/* Reset RX count */
		buf->rx_count = 0;

		/* Set offset to prevent CSP header from being copied to CSP data */
		offset = sizeof(uint32_t) + sizeof(uint16_t);

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

int csp_can1_tx(const csp_route_t * ifroute, csp_packet_t *packet) {

	csp_iface_t * iface = ifroute->iface;
	csp_can_interface_data_t * ifdata = iface->interface_data;

	/* Get an unique CFP id - this should be locked to prevent access from multiple tasks */
	const uint32_t ident = ifdata->cfp_packet_counter++;

	/* Figure out destination node based on routing entry */
	const uint8_t dest = (ifroute->via != CSP_NO_VIA_ADDRESS) ? ifroute->via : packet->id.dst;

	uint32_t can_id = 0;
	uint8_t data_bytes = 0;
	uint8_t frame_buf[CAN_FRAME_SIZE];

	/**
	 * CSP 1.x Frame Header:
	 * Data offset is always 6.
	 */
	can_id = (CFP_MAKE_SRC(packet->id.src) |
					CFP_MAKE_DST(dest) |
					CFP_MAKE_ID(ident) |
					CFP_MAKE_TYPE(CFP_BEGIN) |
					CFP_MAKE_REMAIN((packet->length + CFP1_DATA_OFFSET - 1) / CAN_FRAME_SIZE));

	/**
	 * CSP 1.x Data field
	 *
	 * 4 byte CSP 1.0 header
	 * 2 byte length field
	 * 2 byte data (optional)
	 */

	/* Copy CSP 1.x headers and data: Always 4 bytes */
	csp_id_prepend(packet);
	memcpy(frame_buf + CFP1_CSP_HEADER_OFFSET, packet->frame_begin, CFP1_CSP_HEADER_SIZE);

	/* Copy length field, always 2 bytes */
	uint16_t csp_length_be = csp_hton16(packet->length);
	memcpy(frame_buf + CFP1_DATA_LEN_OFFSET, &csp_length_be, CFP1_DATA_LEN_SIZE);

	/* Calculate number of data bytes. Max 2 bytes possible */
	data_bytes = (packet->length <= CFP1_DATA_SIZE_BEGIN) ? packet->length : CFP1_DATA_SIZE_BEGIN;
	memcpy(frame_buf + CFP1_DATA_OFFSET, packet->data, data_bytes);

	/* Increment tx counter */
	uint16_t tx_count = data_bytes;

	const csp_can_driver_tx_t tx_func = ifdata->tx_func;

	/* Send first frame */
	if ((tx_func)(iface->driver_data, can_id, frame_buf, CFP1_DATA_OFFSET + data_bytes) != CSP_ERR_NONE) {
		//csp_log_warn("Failed to send CAN frame in csp_tx_can");
		iface->tx_error++;
		/* Does not free on return */
		return CSP_ERR_DRIVER;
	}

	/* Send next frames if not complete */
	while (tx_count < packet->length) {

		/**
		 * CSP 1.x Frame Header:
		 * Data offset is always 6.
		 */

		/* Calculate frame data bytes */
		data_bytes = (packet->length - tx_count >= CAN_FRAME_SIZE) ? CAN_FRAME_SIZE : packet->length - tx_count;

		/* Prepare identifier */
		can_id = (CFP_MAKE_SRC(packet->id.src) |
			CFP_MAKE_DST(dest) |
			CFP_MAKE_ID(ident) |
			CFP_MAKE_TYPE(CFP_MORE) |
			CFP_MAKE_REMAIN((packet->length - tx_count - data_bytes + CAN_FRAME_SIZE - 1) / CAN_FRAME_SIZE));

		/* Increment tx counter */
		tx_count += data_bytes;

		/* Send frame */
		if ((tx_func)(iface->driver_data, can_id, packet->data + tx_count - data_bytes, data_bytes) != CSP_ERR_NONE) {
			//csp_log_warn("Failed to send CAN frame in Tx callback");
			iface->tx_error++;
			/* Does not free on return */
			return CSP_ERR_DRIVER;
		}
	}

	csp_buffer_free(packet);

	return CSP_ERR_NONE;
}

int csp_can2_rx(csp_iface_t *iface, uint32_t id, const uint8_t *data, uint8_t dlc, int *task_woken) {

	/* Bind incoming frame to a packet buffer */
	csp_can_pbuf_element_t * buf = csp_can_pbuf_find(id, CFP2_ID_CONN_MASK, task_woken);
	if (buf == NULL) {
		if (id & (CFP2_BEGIN_MASK << CFP2_BEGIN_OFFSET)) {
			buf = csp_can_pbuf_new(id, task_woken);
			if (buf == NULL) {
				//csp_log_warn("No available packet buffer for CAN");
				iface->rx_error++;
				return CSP_ERR_NOMEM;
			}
		} else {
			//csp_log_warn("can: Incomplete fragment\n");;
			return CSP_ERR_INVAL;
		}
	}

	/* BEGIN */
	if (id & (CFP2_BEGIN_MASK << CFP2_BEGIN_OFFSET)) {

		/* Discard packet if DLC is less than CSP id + CSP length fields */
		if (dlc < 4) {
			//csp_log_warn("Short BEGIN frame received");
			iface->frame++;
			csp_can_pbuf_free(buf, task_woken);
			return CSP_ERR_INVAL;
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
				return CSP_ERR_NOBUFS;
			}
		}

		csp_id2_setup_rx(buf->packet);

		/* Copy first 2 bytes from CFP 2.0 header:
		 * Because the id field has already been converted in memory to a 32-bit
		 * host-order field, extract the first two bytes and convert back to
		 * network order */
		uint16_t first_two = id >> CFP2_DST_OFFSET;
		first_two = csp_hton16(first_two);
		memcpy(buf->packet->frame_begin, &first_two, 2);

		/* Copy next 4 from data, the data field is in network order */
		memcpy(&buf->packet->frame_begin[2], data, 4);

		buf->packet->frame_length = 6;
		buf->packet->length = 0;

		/* Move RX offset for incoming data */
		data += 4;
		dlc -= 4;

		/* Set next expected fragment counter to be 1 */
		buf->rx_count = 1;

	/* FRAGMENT */
	} else {

		int fragment_counter = (id >> CFP2_FC_OFFSET) & CFP2_FC_MASK;

		/* Check fragment counter is increasing:
		 * We abuse / reuse the rx_count pbuf field
		 * (Note this could be done using csp buffers instead) */
		if ((buf->rx_count) != fragment_counter) {
			//csp_log_error("CAN frame lost in CSP packet");
			csp_can_pbuf_free(buf, task_woken);
			iface->frame++;
			return CSP_ERR_INVAL;
		}

		/* Increment expected next fragment counter:
		 * and with the mask in order to wrap around */
		buf->rx_count = (buf->rx_count + 1) & CFP2_FC_MASK;

	}

	/* Check for overflow */
	if (buf->packet->length + dlc > iface->mtu) {
		//csp_log_error("RX buffer overflow");
		iface->frame++;
		csp_can_pbuf_free(buf, task_woken);
		return CSP_ERR_INVAL;
	}

	/* Copy dlc bytes into buffer */
	memcpy(&buf->packet->frame_begin[buf->packet->frame_length], data, dlc);
	buf->packet->frame_length += dlc;

	/* END */
	if (id & (CFP2_END_MASK << CFP2_END_OFFSET)) {

		/* Parse CSP header into csp_id type */
		csp_id2_strip(buf->packet);

		/* Data is available */
		csp_qfifo_write(buf->packet, iface, task_woken);

		/* Drop packet buffer reference */
		buf->packet = NULL;

		/* Free packet buffer */
		csp_can_pbuf_free(buf, task_woken);

	}

	return CSP_ERR_NONE;

}

int csp_can2_tx(const csp_route_t * ifroute, csp_packet_t *packet) {

	csp_iface_t * iface = ifroute->iface;
	csp_can_interface_data_t * ifdata = iface->interface_data;

	/* Setup counters */
	int sender_count = ifdata->cfp_packet_counter++;
	int tx_count = 0;

	uint32_t can_id = 0;
	uint8_t frame_buf[CAN_FRAME_SIZE] __attribute__((aligned(sizeof(uint32_t))));
	uint8_t frame_buf_inp = 0;
	uint8_t frame_buf_avail = CAN_FRAME_SIZE;

	/* Pack mandatory fields of header */
	can_id = (((packet->id.pri & CFP2_PRIO_MASK) << CFP2_PRIO_OFFSET) |
	          ((packet->id.dst & CFP2_DST_MASK) << CFP2_DST_OFFSET) |
	          ((csp_conf.address & CFP2_SENDER_MASK) << CFP2_SENDER_OFFSET) |
	          ((sender_count & CFP2_SC_MASK) << CFP2_SC_OFFSET) |
	          ((1 & CFP2_BEGIN_MASK) << CFP2_BEGIN_OFFSET));

	/* Pack the rest of the CSP header in the first 32-bit of data */
	uint32_t * header_extension = (uint32_t *) frame_buf;

	*header_extension = (((packet->id.src & CFP2_SRC_MASK) << CFP2_SRC_OFFSET) |
	                     ((packet->id.dport & CFP2_DPORT_MASK) << CFP2_DPORT_OFFSET) |
	                     ((packet->id.sport & CFP2_SPORT_MASK) << CFP2_SPORT_OFFSET) |
	                     ((packet->id.flags & CFP2_FLAGS_MASK) << CFP2_FLAGS_OFFSET));

	/* Convert to network byte order */
	*header_extension = csp_hton32(*header_extension);

	frame_buf_inp += 4;
	frame_buf_avail -= 4;

	/* Copy first bytes of data field (max 4) */
	int data_bytes = (packet->length >= 4) ? 4 : packet->length;
	memcpy(frame_buf + frame_buf_inp, packet->data, data_bytes);
	frame_buf_inp += data_bytes;
	tx_count = data_bytes;

	/* Check for end condition */
	if (tx_count == packet->length) {
		can_id |= ((1 & CFP2_END_MASK) << CFP2_END_OFFSET);
	}

	/* Send first frame now */
	if ((ifdata->tx_func)(iface->driver_data, can_id, frame_buf, frame_buf_inp) != CSP_ERR_NONE) {
		iface->tx_error++;
		/* Does not free on return */
		return CSP_ERR_DRIVER;
	}

	/* Send next fragments if not complete */
	int fragment_count = 1;
	while (tx_count < packet->length) {

		/* Pack mandatory fields of header */
		can_id = (((packet->id.pri & CFP2_PRIO_MASK) << CFP2_PRIO_OFFSET) |
				  ((packet->id.dst & CFP2_DST_MASK) << CFP2_DST_OFFSET) |
				  ((csp_conf.address & CFP2_SENDER_MASK) << CFP2_SENDER_OFFSET) |
				  ((sender_count & CFP2_SC_MASK) << CFP2_SC_OFFSET));

		/* Set and increment fragment count */
		can_id |= (fragment_count++ & CFP2_FC_MASK) << CFP2_FC_OFFSET;

		/* Calculate frame data bytes */
		data_bytes = (packet->length - tx_count >= CAN_FRAME_SIZE) ? CAN_FRAME_SIZE : packet->length - tx_count;

		/* Check for end condition */
		if (tx_count + data_bytes == packet->length) {
			can_id |= ((1 & CFP2_END_MASK) << CFP2_END_OFFSET);
		}

		/* Send frame */
		if ((ifdata->tx_func)(iface->driver_data, can_id, packet->data + tx_count, data_bytes) != CSP_ERR_NONE) {
			//csp_log_warn("Failed to send CAN frame in Tx callback");
			iface->tx_error++;
			/* Does not free on return */
			return CSP_ERR_DRIVER;
		}

		/* Increment tx counter */
		tx_count += data_bytes;

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

	/* We reserve 8 bytes of the data field, for CFP information:
	 * In reality we dont use as much, its between 3 and 6 depending
	 * on CFP format.
	 */
	iface->mtu = csp_buffer_data_size() - 8;

	ifdata->cfp_packet_counter = 0;

	if (csp_conf.version == 1) {
		iface->nexthop = csp_can1_tx;
	} else {
		iface->nexthop = csp_can2_tx;
	}

	return csp_iflist_add(iface);
}

int csp_can_rx(csp_iface_t *iface, uint32_t id, const uint8_t *data, uint8_t dlc, int *task_woken) {
	if (csp_conf.version == 1) {
		return csp_can1_rx(iface, id, data, dlc, task_woken);
	} else {
		return csp_can2_rx(iface, id, data, dlc, task_woken);
	}
}
