#include <csp/interfaces/csp_if_canfd.h>

#include <string.h>
#include <stdlib.h>
#include <endian.h>

#include <csp/csp.h>
#include <csp/csp_id.h>

#include "csp/csp_types.h"
#include "csp_if_can_pbuf.h"

/* Max number of bytes per classic CAN frame */
#define CAN_FRAME_SIZE 8

/* CSP header bytes in the first frame */
#define CFP2_HEADER_EXTENSION_SIZE 4

/* Largest valid CAN FD frame size that can be transmitted from bytes */
static uint8_t csp_canfd_frame_size(unsigned int bytes) {

	/* Valid CAN FD frame sizes above 8 bytes */
	static const uint8_t fd_sizes[] = {64, 48, 32, 24, 20, 16, 12};

	if (bytes <= CAN_FRAME_SIZE) {
		return bytes;
	}

	for (unsigned int i = 0; i < sizeof(fd_sizes); i++) {
		if (fd_sizes[i] <= bytes) {
			return fd_sizes[i];
		}
	}

	/* bytes is in the range 9..11 */
	return CAN_FRAME_SIZE;
}

static int csp_canfd_tx(csp_iface_t * iface, uint16_t via, csp_packet_t * packet, int from_me) {
	/* Avoid compiler warnings about unused parameter */
	(void)via;
	(void)from_me;

	/* Loopback */
	if (packet->id.dst == iface->addr || csp_addr_is_alias(packet->id.dst)) {
		csp_qfifo_write(packet, iface, NULL);
		return CSP_ERR_NONE;
	}

	csp_can_interface_data_t * ifdata = iface->interface_data;

	/* Setup counters */
	int sender_count = ifdata->cfp_packet_counter++;  // Atomic operation as cfp_packet_counter is of type atomic_int
	int tx_count = 0;

	uint32_t can_id = 0;
	uint8_t frame_buf_inp = 0;
	const csp_packet_t * ctx_packet = NULL;

	/* Pack mandatory fields of header */
	can_id = ((((uint32_t)(packet->id.pri & CFP2_PRIO_MASK)) << CFP2_PRIO_OFFSET) |
			  (((uint32_t)(packet->id.dst & CFP2_DST_MASK)) << CFP2_DST_OFFSET) |
			  (((uint32_t)(iface->addr & CFP2_SENDER_MASK)) << CFP2_SENDER_OFFSET) |
			  (((uint32_t)(sender_count & CFP2_SC_MASK)) << CFP2_SC_OFFSET) |
			  (((uint32_t)(1 & CFP2_BEGIN_MASK)) << CFP2_BEGIN_OFFSET));

	/* Pack the rest of the CSP header in the first 32-bit of data */
	uint32_t frame_buf_mem[(CSP_CANFD_FRAME_SIZE + sizeof(uint32_t) - 1) / sizeof(uint32_t)];
	uint8_t * frame_buf = (uint8_t *)frame_buf_mem;
	uint32_t * header_extension = (uint32_t *)frame_buf_mem;

	*header_extension = ((((uint32_t)(packet->id.src & CFP2_SRC_MASK)) << CFP2_SRC_OFFSET) |
						 (((uint32_t)(packet->id.dport & CFP2_DPORT_MASK)) << CFP2_DPORT_OFFSET) |
						 (((uint32_t)(packet->id.sport & CFP2_SPORT_MASK)) << CFP2_SPORT_OFFSET) |
						 (((uint32_t)(packet->id.flags & CFP2_FLAGS_MASK)) << CFP2_FLAGS_OFFSET));

	/* Convert to network byte order */
	*header_extension = htobe32(*header_extension);

	frame_buf_inp += CFP2_HEADER_EXTENSION_SIZE;

	/* Copy first bytes of data field, rounded down to a valid frame size */
	int data_bytes = csp_canfd_frame_size(CFP2_HEADER_EXTENSION_SIZE + packet->length) - CFP2_HEADER_EXTENSION_SIZE;
	memcpy(frame_buf + frame_buf_inp, packet->data, data_bytes);
	frame_buf_inp += data_bytes;
	tx_count = data_bytes;

	/* Check for end condition */
	if (tx_count == packet->length) {
		can_id |= ((uint32_t)(1 & CFP2_END_MASK)) << CFP2_END_OFFSET;
		ctx_packet = packet;
	}

	/* Send first frame now */
	if ((ifdata->tx_func)(iface->driver_data, can_id, frame_buf, frame_buf_inp, ctx_packet) != CSP_ERR_NONE) {
		iface->tx_error++;
		/* Does not free on return */
		return CSP_ERR_DRIVER;
	}

	/* Send next fragments if not complete */
	int fragment_count = 1;
	while (tx_count < packet->length) {

		/* Pack mandatory fields of header */
		can_id = ((((uint32_t)(packet->id.pri & CFP2_PRIO_MASK)) << CFP2_PRIO_OFFSET) |
				  (((uint32_t)(packet->id.dst & CFP2_DST_MASK)) << CFP2_DST_OFFSET) |
				  (((uint32_t)(iface->addr & CFP2_SENDER_MASK)) << CFP2_SENDER_OFFSET) |
				  (((uint32_t)(sender_count & CFP2_SC_MASK)) << CFP2_SC_OFFSET));

		/* Set and increment fragment count */
		can_id |= ((uint32_t)(fragment_count++ & CFP2_FC_MASK)) << CFP2_FC_OFFSET;

		/* Calculate frame data bytes, rounded down to a valid frame size */
		data_bytes = csp_canfd_frame_size(packet->length - tx_count);

		/* Check for end condition */
		if (tx_count + data_bytes == packet->length) {
			can_id |= ((uint32_t)(1 & CFP2_END_MASK)) << CFP2_END_OFFSET;
			ctx_packet = packet;
		}

		if ((ifdata->tx_func)(iface->driver_data, can_id, packet->data + tx_count, data_bytes, ctx_packet) != CSP_ERR_NONE) {
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

int csp_canfd_rx(csp_iface_t * iface, uint32_t id, const uint8_t * data, uint8_t data_size, uint32_t timestamp_rx, int * task_woken) {

	csp_can_interface_data_t * ifdata = iface->interface_data;

	/* Discard frames larger than a CAN FD frame */
	if (data_size > CSP_CANFD_FRAME_SIZE) {
		iface->frame++;
		return CSP_ERR_INVAL;
	}

	/* Bind incoming frame to a packet buffer */
	csp_packet_t * packet = csp_can_pbuf_find(ifdata, id, CFP2_ID_CONN_MASK, task_woken);
	if (packet == NULL) {
		if (id & (CFP2_BEGIN_MASK << CFP2_BEGIN_OFFSET)) {

			/* Discard packet if the first frame is shorter than the CSP header extension */
			if (data_size < CFP2_HEADER_EXTENSION_SIZE) {
				csp_dbg_can_errno = CSP_DBG_CAN_ERR_SHORT_BEGIN;
				iface->frame++;
				return CSP_ERR_INVAL;
			}

			/* Copy first 2 bytes from CFP 2.0 header:
			 * Because the id field has already been converted in memory to a 32-bit
			 * host-order field, extract the first two bytes and convert back to
			 * network order */
			uint8_t header[6];
			uint16_t first_two = id >> CFP2_DST_OFFSET;
			first_two = htobe16(first_two);
			memcpy(header, &first_two, 2);

			/* Copy next 4 from data, the data field is in network order */
			memcpy(&header[2], data, CFP2_HEADER_EXTENSION_SIZE);

			/* Move RX offset for incoming data */
			data += CFP2_HEADER_EXTENSION_SIZE;
			data_size -= CFP2_HEADER_EXTENSION_SIZE;

			/* Create CSP header info from the first bytes received */
			csp_id_t csp_id = csp_id_extract(header);
			packet = csp_can_pbuf_new(ifdata, id, csp_id, task_woken);
			if (packet == NULL) {
				iface->drop++;
				return CSP_ERR_NOBUFS;
			}

			/* Prepare new CSP packet by adding header as extracted */
			csp_id_setup_rx(packet);
			packet->id = csp_id;
			memcpy(packet->frame_begin, header, csp_id_get_header_size());
			packet->frame_length = csp_id_get_header_size();
			packet->length = 0;

			/* Set next expected fragment counter to be 1 */
			packet->rx_count = 1;
		} else {
			iface->frame++;
			return CSP_ERR_INVAL;
		}
	}

	/* FRAGMENT */
	if (!(id & (CFP2_BEGIN_MASK << CFP2_BEGIN_OFFSET))) {

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

	/* Check for overflow. The frame input + data_size must not exceed the end of the packet data field */
	if (&packet->frame_begin[packet->frame_length] + data_size > &packet->data[sizeof(packet->data)]) {
		csp_dbg_can_errno = CSP_DBG_CAN_ERR_RX_OVF;
		iface->rx_error++;
		csp_can_pbuf_free(ifdata, packet, 1, task_woken);
		return CSP_ERR_INVAL;
	}

	/* Copy data_size bytes into buffer */
	memcpy(&packet->frame_begin[packet->frame_length], data, data_size);
	packet->frame_length += data_size;

	/* END */
	if (id & (CFP2_END_MASK << CFP2_END_OFFSET)) {

		/* Remember the interface timestamp for the last CFP fragment */
		packet->timestamp_rx = timestamp_rx;

		/* Extract data length */
		packet->length = packet->frame_length - csp_id_get_header_size();

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

int csp_canfd_add_interface(csp_iface_t * iface) {

	if ((iface == NULL) || (iface->name == NULL) || (iface->interface_data == NULL)) {
		return CSP_ERR_INVAL;
	}

	csp_can_interface_data_t * ifdata = iface->interface_data;
	if (ifdata->tx_func == NULL) {
		return CSP_ERR_INVAL;
	}

	/* CAN FD requires CFP 2.x */
	if (csp_conf.version != 2) {
		return CSP_ERR_NOTSUP;
	}

	ifdata->cfp_packet_counter = 0;

	iface->nexthop = csp_canfd_tx;

	csp_iflist_add(iface);

	return CSP_ERR_NONE;
}

int csp_canfd_remove_interface(csp_iface_t * iface) {

	if (iface == NULL) {
		return CSP_ERR_INVAL;
	}

	csp_iflist_remove(iface);

	return CSP_ERR_NONE;
}
