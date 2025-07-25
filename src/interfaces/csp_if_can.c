

#include <csp/interfaces/csp_if_can.h>

#include <string.h>
#include <stdlib.h>
#include <endian.h>

#include <csp/csp.h>
#include <csp/csp_id.h>

#include "csp_if_can_pbuf.h"

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
#define CFP1_CSP_HEADER_SIZE   4
#define CFP1_DATA_LEN_OFFSET   4
#define CFP1_DATA_LEN_SIZE     2
#define CFP1_DATA_OFFSET       6
#define CFP1_DATA_SIZE_BEGIN   2
#define CFP1_DATA_SIZE_MORE    8

/* CFP type */
enum cfp_frame_t {
	/* First CFP fragment of a CSP packet */
	CFP_BEGIN = 0,
	/* Remaining CFP fragment(s) of a CSP packet */
	CFP_MORE = 1
};

int csp_can1_rx(csp_iface_t * iface, uint32_t id, const uint8_t * data, uint8_t dlc, int * task_woken) {

	/* Test: random packet loss */
	// if (0) {
	// 	int random = rand();
	// 	if (random < RAND_MAX * 0.00005) {
	// 		return CSP_ERR_DRIVER;
	// 	}
	// }

	csp_can_interface_data_t * ifdata = iface->interface_data;

	/* Bind incoming frame to a packet buffer */
	csp_packet_t * packet = csp_can_pbuf_find(ifdata, id, CFP_ID_CONN_MASK, task_woken);
	if (packet == NULL) {
		if (CFP_TYPE(id) == CFP_BEGIN) {
			packet = csp_can_pbuf_new(ifdata, id, task_woken);
		} else {
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
				csp_dbg_can_errno = CSP_DBG_CAN_ERR_SHORT_BEGIN;
				iface->frame++;
				csp_can_pbuf_free(ifdata, packet, 1, task_woken);
				break;
			}

			csp_id_setup_rx(packet);

			/* Copy CSP identifier (header) */
			memcpy(packet->frame_begin, data, sizeof(uint32_t));
			packet->frame_length += sizeof(uint32_t);

			csp_id_strip(packet);

			/* Copy CSP length (of data) */
			memcpy(&(packet->length), data + sizeof(uint32_t), sizeof(packet->length));
			packet->length = be16toh(packet->length);

			/* Overflow: check if incoming frame data length is larger than buffer length  */
			if (packet->length > sizeof(packet->data)) {
				iface->rx_error++;
				csp_can_pbuf_free(ifdata, packet, 1, task_woken);
				break;
			}

			/* Reset RX count */
			packet->rx_count = 0;

			/* Set offset to prevent CSP header from being copied to CSP data */
			offset = sizeof(uint32_t) + sizeof(uint16_t);

			/* Set remain field - increment to include begin packet */
			packet->remain = CFP_REMAIN(id) + 1;

			/* FALLTHROUGH */

		case CFP_MORE:

			/* Check 'remain' field match */
			if ((uint16_t) CFP_REMAIN(id) != packet->remain - 1) {
				csp_dbg_can_errno = CSP_DBG_CAN_ERR_FRAME_LOST;
				csp_can_pbuf_free(ifdata, packet, 1, task_woken);
				iface->frame++;
				break;
			}

			/* Decrement remaining frames */
			packet->remain--;

			/* Check for overflow */
			if ((packet->rx_count + dlc - offset) > packet->length) {
				csp_dbg_can_errno = CSP_DBG_CAN_ERR_RX_OVF;
				iface->frame++;
				csp_can_pbuf_free(ifdata, packet, 1, task_woken);
				break;
			}

			/* Copy dlc bytes into buffer */
			memcpy(&packet->data[packet->rx_count], data + offset, dlc - offset);
			packet->rx_count += dlc - offset;

			/* Check if more data is expected */
			if (packet->rx_count != packet->length)
				break;

			/* Rewrite incoming L2 broadcast to local node */
			if (packet->id.dst == 0x1F) {
				packet->id.dst = iface->addr;
			}

			/* Free packet buffer */
			csp_can_pbuf_free(ifdata, packet, 0, task_woken);

			/* Data is available */
			csp_qfifo_write(packet, iface, task_woken);

			break;

		default:
			csp_dbg_can_errno = CSP_DBG_CAN_ERR_UNKNOWN;
			csp_can_pbuf_free(ifdata, packet, 1, task_woken);
			break;
	}

	return CSP_ERR_NONE;
}

int csp_can1_tx(csp_iface_t * iface, uint16_t via, csp_packet_t * packet, int from_me) {

	/* Loopback */
	if (packet->id.dst == iface->addr) {
		csp_qfifo_write(packet, iface, NULL);
		return CSP_ERR_NONE;
	}

	csp_can_interface_data_t * ifdata = iface->interface_data;

	/* Get an unique CFP id - this should be locked to prevent access from multiple tasks */
	const uint32_t ident = ifdata->cfp_packet_counter++;

	/* Figure out destination node based on routing entry */
	const uint8_t dest = (via != CSP_NO_VIA_ADDRESS) ? via : packet->id.dst;

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
	uint16_t csp_length_be = htobe16(packet->length);
	memcpy(frame_buf + CFP1_DATA_LEN_OFFSET, &csp_length_be, CFP1_DATA_LEN_SIZE);

	/* Calculate number of data bytes. Max 2 bytes possible */
	data_bytes = (packet->length <= CFP1_DATA_SIZE_BEGIN) ? packet->length : CFP1_DATA_SIZE_BEGIN;
	memcpy(frame_buf + CFP1_DATA_OFFSET, packet->data, data_bytes);

	/* Increment tx counter */
	uint16_t tx_count = data_bytes;

	const csp_can_driver_tx_t tx_func = ifdata->tx_func;

	/* Send first frame */
	if ((tx_func)(iface->driver_data, can_id, frame_buf, CFP1_DATA_OFFSET + data_bytes) != CSP_ERR_NONE) {
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
			iface->tx_error++;
			/* Does not free on return */
			return CSP_ERR_DRIVER;
		}
	}

	csp_buffer_free(packet);

	return CSP_ERR_NONE;
}

int csp_can2_rx(csp_iface_t * iface, uint32_t id, const uint8_t * data, uint8_t dlc, int * task_woken) {

	csp_can_interface_data_t * ifdata = iface->interface_data;

	/* Bind incoming frame to a packet buffer */
	csp_packet_t * packet = csp_can_pbuf_find(ifdata, id, CFP2_ID_CONN_MASK, task_woken);
	if (packet == NULL) {
		if (id & (CFP2_BEGIN_MASK << CFP2_BEGIN_OFFSET)) {
			packet = csp_can_pbuf_new(ifdata, id, task_woken);
		} else {
			iface->frame++;
			return CSP_ERR_INVAL;
		}
	}


	/* BEGIN */
	if (id & (CFP2_BEGIN_MASK << CFP2_BEGIN_OFFSET)) {

		/* Discard packet if DLC is less than CSP id + CSP length fields */
		if (dlc < 4) {
			csp_dbg_can_errno = CSP_DBG_CAN_ERR_SHORT_BEGIN;
			iface->frame++;
			csp_can_pbuf_free(ifdata, packet, 1, task_woken);
			return CSP_ERR_INVAL;
		}

		csp_id_setup_rx(packet);

		/* Copy first 2 bytes from CFP 2.0 header:
		 * Because the id field has already been converted in memory to a 32-bit
		 * host-order field, extract the first two bytes and convert back to
		 * network order */
		uint16_t first_two = id >> CFP2_DST_OFFSET;
		first_two = htobe16(first_two);
		memcpy(packet->frame_begin, &first_two, 2);

		/* Copy next 4 from data, the data field is in network order */
		memcpy(&packet->frame_begin[2], data, 4);

		packet->frame_length = 6;
		packet->length = 0;

		/* Move RX offset for incoming data */
		data += 4;
		dlc -= 4;

		/* Set next expected fragment counter to be 1 */
		packet->rx_count = 1;

		/* FRAGMENT */
	} else {

		int fragment_counter = (id >> CFP2_FC_OFFSET) & CFP2_FC_MASK;

		/* Check fragment counter is increasing:
		 * We abuse / reuse the rx_count pbuf field
		 * (Note this could be done using csp buffers instead) */
		if ((packet->rx_count) != fragment_counter) {
			csp_dbg_can_errno = CSP_DBG_CAN_ERR_FRAME_LOST;
			csp_can_pbuf_free(ifdata, packet, 1, task_woken);
			iface->frame++;
			return CSP_ERR_INVAL;
		}

		/* Increment expected next fragment counter:
		 * and with the mask in order to wrap around */
		packet->rx_count = (packet->rx_count + 1) & CFP2_FC_MASK;
	}

	/* Check for overflow. The frame input + dlc must not exceed the end of the packet data field */
	if (&packet->frame_begin[packet->frame_length] + dlc > &packet->data[sizeof(packet->data)]) {
		csp_dbg_can_errno = CSP_DBG_CAN_ERR_RX_OVF;
		iface->rx_error++;
		csp_can_pbuf_free(ifdata, packet, 1, task_woken);
		return CSP_ERR_INVAL;
	}

	/* Copy dlc bytes into buffer */
	memcpy(&packet->frame_begin[packet->frame_length], data, dlc);
	packet->frame_length += dlc;

	/* END */
	if (id & (CFP2_END_MASK << CFP2_END_OFFSET)) {

		/* Parse CSP header into csp_id type */
		csp_id_strip(packet);

		/* Rewrite incoming L2 broadcast to local node */
		if (packet->id.dst == 0x3FFF) {
			packet->id.dst = iface->addr;
		}

		/* Free packet buffer */
		csp_can_pbuf_free(ifdata, packet, 0, task_woken);

		/* Data is available */
		csp_qfifo_write(packet, iface, task_woken);

	}

	return CSP_ERR_NONE;
}

int csp_can2_tx(csp_iface_t * iface, uint16_t via, csp_packet_t * packet, int from_me) {

	/* Loopback */
	if (packet->id.dst == iface->addr) {
		csp_qfifo_write(packet, iface, NULL);
		return CSP_ERR_NONE;
	}

	csp_can_interface_data_t * ifdata = iface->interface_data;

	/* Setup counters */
	int sender_count = ifdata->cfp_packet_counter++;
	int tx_count = 0;

	uint32_t can_id = 0;
	uint8_t frame_buf_inp = 0;

	/* Pack mandatory fields of header */
	can_id = (((packet->id.pri & CFP2_PRIO_MASK) << CFP2_PRIO_OFFSET) |
			  ((packet->id.dst & CFP2_DST_MASK) << CFP2_DST_OFFSET) |
			  ((iface->addr & CFP2_SENDER_MASK) << CFP2_SENDER_OFFSET) |
			  ((sender_count & CFP2_SC_MASK) << CFP2_SC_OFFSET) |
			  ((1 & CFP2_BEGIN_MASK) << CFP2_BEGIN_OFFSET));

	/* Pack the rest of the CSP header in the first 32-bit of data */
    uint32_t frame_buf_mem[(CAN_FRAME_SIZE+sizeof(uint32_t)-1)/sizeof(uint32_t)];
    uint8_t *frame_buf = (uint8_t*)frame_buf_mem;
	uint32_t * header_extension = (uint32_t *)frame_buf_mem;

	*header_extension = (((packet->id.src & CFP2_SRC_MASK) << CFP2_SRC_OFFSET) |
						 ((packet->id.dport & CFP2_DPORT_MASK) << CFP2_DPORT_OFFSET) |
						 ((packet->id.sport & CFP2_SPORT_MASK) << CFP2_SPORT_OFFSET) |
						 ((packet->id.flags & CFP2_FLAGS_MASK) << CFP2_FLAGS_OFFSET));

	/* Convert to network byte order */
	*header_extension = htobe32(*header_extension);

	frame_buf_inp += 4;

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
				  ((iface->addr & CFP2_SENDER_MASK) << CFP2_SENDER_OFFSET) |
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

	ifdata->cfp_packet_counter = 0;

	if (csp_conf.version == 1) {
		iface->nexthop = csp_can1_tx;
	} else {
		iface->nexthop = csp_can2_tx;
	}

	csp_iflist_add(iface);

	return CSP_ERR_NONE;
}

int csp_can_remove_interface(csp_iface_t * iface) {

	if (iface == NULL) {
		return CSP_ERR_INVAL;
	}

	csp_iflist_remove(iface);

	return CSP_ERR_NONE;
}

int csp_can_rx(csp_iface_t * iface, uint32_t id, const uint8_t * data, uint8_t dlc, int * task_woken) {
	if (csp_conf.version == 1) {
		return csp_can1_rx(iface, id, data, dlc, task_woken);
	} else {
		return csp_can2_rx(iface, id, data, dlc, task_woken);
	}
}
