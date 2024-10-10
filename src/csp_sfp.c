#include <csp/csp_sfp.h>

#include <stdlib.h>

#include <csp/csp_buffer.h>
#include <csp/csp_debug.h>
#include "csp_macro.h"
#include <endian.h>

#include "csp_conn.h"

typedef struct __packed {
	uint32_t offset;
	uint32_t totalsize;
} sfp_header_t;

/**
 * SFP Headers:
 * The following functions are helper functions that handles the extra SFP
 * information that needs to be appended to all data packets.
 */
static inline sfp_header_t * csp_sfp_header_add(csp_packet_t * packet) {

	sfp_header_t * header = (sfp_header_t *)&packet->data[packet->length];
	packet->length += sizeof(*header);
	return header;
}

static inline sfp_header_t * csp_sfp_header_remove(csp_packet_t * packet) {

	if ((packet->id.flags & CSP_FFRAG) == 0) {
		return NULL;
	}
	sfp_header_t * header;
	if (packet->length < sizeof(*header)) {
		return NULL;
	}
	header = (sfp_header_t *)&packet->data[packet->length - sizeof(*header)];
	packet->length -= sizeof(*header);

	header->offset = be32toh(header->offset);
	header->totalsize = be32toh(header->totalsize);

	if (header->offset > header->totalsize) {
		return NULL;
	}

	return header;
}

int csp_sfp_send_own_memcpy(csp_conn_t * conn, const void * data, unsigned int totalsize, unsigned int mtu, uint32_t timeout, csp_memcpy_fnc_t memcpyfcn) {
	if (mtu == 0 || mtu + sizeof(sfp_header_t) > CSP_BUFFER_SIZE) {
		return CSP_ERR_INVAL;
	}

	unsigned int count = 0;
	while ((count < totalsize) && csp_conn_is_active(conn)) {

		sfp_header_t * sfp_header;

		/* Allocate packet */
		csp_packet_t * packet = csp_buffer_get(0);
		if (packet == NULL) {
			return CSP_ERR_NOMEM;
		}

		/* Calculate sending size */
		unsigned int size = totalsize - count;
		if (size > mtu) {
			size = mtu;
		}

		/* Print debug */
		//csp_print("%s: %d:%d, sending at %p size %u\n", __func__, csp_conn_src(conn), csp_conn_sport(conn), (void *)((uint8_t *)data + count), size);

		/* Copy data */
		(memcpyfcn)((csp_memptr_t)(uintptr_t)packet->data, (csp_memptr_t)(uintptr_t)(((uint8_t *)data) + count), size);
		packet->length = size;

		/* Set fragment flag */
		conn->idout.flags |= CSP_FFRAG;

		/* Add SFP header */
		sfp_header = csp_sfp_header_add(packet);
		sfp_header->totalsize = htobe32(totalsize);
		sfp_header->offset = htobe32(count);

		/* Send data */
		csp_send(conn, packet);

		/* Increment count */
		count += size;
	}

	return CSP_ERR_NONE;
}

int csp_sfp_recv_fp(csp_conn_t * conn, void ** return_data, int * return_datasize, uint32_t timeout, csp_packet_t * first_packet) {

	*return_data = NULL; /* Allow caller to assume csp_free() can always be called when dataout is non-NULL */
	*return_datasize = 0;

	/* Get first packet from user, or from connection */
	csp_packet_t * packet;
	if (first_packet == NULL) {
		packet = csp_read(conn, timeout);
		if (packet == NULL) {
			return CSP_ERR_TIMEDOUT;
		}
	} else {
		packet = first_packet;
	}

	uint8_t * data = NULL;
	uint32_t datasize = 0;
	uint32_t data_offset = 0;
	int error = CSP_ERR_TIMEDOUT;
	do {
		/* Read SFP header */
		sfp_header_t * sfp_header = csp_sfp_header_remove(packet);
		if (sfp_header == NULL) {
			//csp_print("%s: %u:%u, invalid message, id.flags: 0x%x, length: %u\n", __func__, packet->id.src, packet->id.sport, packet->id.flags, packet->length);
			csp_buffer_free(packet);

			error = CSP_ERR_SFP;
			goto error;
		}

		//csp_print("%s: %u:%u, fragment %" PRIu32 "/%" PRIu32 "\n",  __func__, packet->id.src, packet->id.sport, sfp_header->offset + packet->length, sfp_header->totalsize);

		/* Consistency check */
		if (sfp_header->offset != data_offset) {
			//csp_print("%s: %u:%u, invalid message, offset %" PRIu32 " (expected %" PRIu32 "), length: %u, totalsize %" PRIu32 "\n", __func__, packet->id.src, packet->id.sport, sfp_header->offset, data_offset, packet->length, sfp_header->totalsize);
			csp_buffer_free(packet);

			error = CSP_ERR_SFP;
			goto error;
		}

		/* Allocate memory */
		if (data == NULL) {
			datasize = sfp_header->totalsize;
			data = malloc(datasize);
			if (data == NULL) {
				//csp_print("%s: %u:%u, malloc(%" PRIu32 ") failed\n", __func__, packet->id.src, packet->id.sport, datasize);
				csp_buffer_free(packet);

				error = CSP_ERR_NOMEM;
				goto error;
			}
		}

		/* Consistency check */
		if (((data_offset + packet->length) > datasize) || (datasize != sfp_header->totalsize)) {
			//csp_print("%s: %u:%u, invalid size, sfp.offset: %" PRIu32 ", length: %u, total: %" PRIu32 " / %" PRIu32 "\n", __func__, packet->id.src, packet->id.sport, sfp_header->offset, packet->length, datasize, sfp_header->totalsize);
			csp_buffer_free(packet);

			error = CSP_ERR_SFP;
			goto error;
		}

		/* Copy data to output */
		memcpy(data + data_offset, packet->data, packet->length);
		data_offset += packet->length;

		if (data_offset >= datasize) {
			// transfer complete
			csp_buffer_free(packet);

			*return_data = data;  // must be freed by csp_free()
			*return_datasize = datasize;
			return CSP_ERR_NONE;
		}

		/* Consistency check */
		if (packet->length == 0) {
			//csp_print("%s: %u:%u, invalid size, sfp.offset: %" PRIu32 ", length: %u, total: %" PRIu32 " / %" PRIu32 "\n", __func__, packet->id.src, packet->id.sport, sfp_header->offset, packet->length, datasize, sfp_header->totalsize);
			csp_buffer_free(packet);

			error = CSP_ERR_SFP;
			goto error;
		}

		csp_buffer_free(packet);

	} while ((packet = csp_read(conn, timeout)) != NULL);

error:
	free(data);
	return error;
}
