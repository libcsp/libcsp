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

#include <csp/csp_sfp.h>

#include <csp/csp_buffer.h>
#include <csp/csp_debug.h>
#include <csp/csp_endian.h>
#include <csp/arch/csp_malloc.h>

#include "csp_conn.h"

typedef struct __attribute__((__packed__)) {
	uint32_t offset;
	uint32_t totalsize;
} sfp_header_t;

/**
 * SFP Headers:
 * The following functions are helper functions that handles the extra SFP
 * information that needs to be appended to all data packets.
 */
static inline sfp_header_t * csp_sfp_header_add(csp_packet_t * packet) {

	sfp_header_t * header = (sfp_header_t *) &packet->data[packet->length];
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
        header = (sfp_header_t *) &packet->data[packet->length - sizeof(*header)];
	packet->length -= sizeof(*header);

	header->offset = csp_ntoh32(header->offset);
	header->totalsize = csp_ntoh32(header->totalsize);

	if (header->offset > header->totalsize) {
		return NULL;
	}

	return header;
}

int csp_sfp_send_own_memcpy(csp_conn_t * conn, const void * data, unsigned int totalsize, unsigned int mtu, uint32_t timeout, csp_memcpy_fnc_t memcpyfcn) {
	if (mtu == 0) {
		return CSP_ERR_INVAL;
	}

	unsigned int count = 0;
	while(count < totalsize) {

		sfp_header_t * sfp_header;

		/* Allocate packet */
		csp_packet_t * packet = csp_buffer_get(mtu + sizeof(*sfp_header));
		if (packet == NULL) {
			return CSP_ERR_NOMEM;
		}

		/* Calculate sending size */
		unsigned int size = totalsize - count;
		if (size > mtu) {
			size = mtu;
		}

		/* Print debug */
		csp_log_protocol("%s: %d:%d, sending at %p size %u",
					__FUNCTION__, csp_conn_src(conn), csp_conn_sport(conn),
					((uint8_t*)data) + count, size);

		/* Copy data */
		(memcpyfcn)((csp_memptr_t)(uintptr_t)packet->data, (csp_memptr_t)(uintptr_t)(((uint8_t*)data) + count), size);
		packet->length = size;

		/* Set fragment flag */
		conn->idout.flags |= CSP_FFRAG;

		/* Add SFP header */
		sfp_header = csp_sfp_header_add(packet); // no check, because buffer was allocated with extra size.
		sfp_header->totalsize = csp_hton32(totalsize);
		sfp_header->offset = csp_hton32(count);

		/* Send data */
		if (!csp_send(conn, packet, timeout)) {
			csp_buffer_free(packet);
			return CSP_ERR_TX;
		}

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
			csp_log_warn("%s: %u:%u, invalid message, id.flags: 0x%x, length: %u",
					__FUNCTION__, packet->id.src, packet->id.sport,
					packet->id.flags, packet->length);
			csp_buffer_free(packet);

			error = CSP_ERR_SFP;
			goto error;
		}

		csp_log_protocol("%s: %u:%u, fragment %"PRIu32"/%"PRIu32,
					__FUNCTION__, packet->id.src, packet->id.sport,
					sfp_header->offset + packet->length, sfp_header->totalsize);

		/* Consistency check */
		if (sfp_header->offset != data_offset) {
			csp_log_warn("%s: %u:%u, invalid message, offset %"PRIu32" (expected %"PRIu32"), length: %u, totalsize %"PRIu32,
					__FUNCTION__, packet->id.src, packet->id.sport,
					sfp_header->offset, data_offset, packet->length, sfp_header->totalsize);
			csp_buffer_free(packet);

			error = CSP_ERR_SFP;
			goto error;
		}

		/* Allocate memory */
		if (data == NULL) {
                        datasize = sfp_header->totalsize;
			data = csp_malloc(datasize);
			if (data == NULL) {
				csp_log_warn("%s: %u:%u, csp_malloc(%"PRIu32") failed",
					__FUNCTION__, packet->id.src, packet->id.sport,
					datasize);
				csp_buffer_free(packet);

				error = CSP_ERR_NOMEM;
				goto error;
			}
		}

		/* Consistency check */
		if (((data_offset + packet->length) > datasize) || (datasize != sfp_header->totalsize)) {
			csp_log_warn("%s: %u:%u, invalid size, sfp.offset: %"PRIu32", length: %u, total: %"PRIu32" / %"PRIu32"",
					__FUNCTION__, packet->id.src, packet->id.sport,
					sfp_header->offset, packet->length, datasize, sfp_header->totalsize);
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

                        *return_data = data; // must be freed by csp_free()
                        *return_datasize = datasize;
			return CSP_ERR_NONE;
		}

		/* Consistency check */
		if (packet->length == 0) {
			csp_log_warn("%s: %u:%u, invalid size, sfp.offset: %"PRIu32", length: %u, total: %"PRIu32" / %"PRIu32"",
					__FUNCTION__, packet->id.src, packet->id.sport,
					sfp_header->offset, packet->length, datasize, sfp_header->totalsize);
			csp_buffer_free(packet);

			error = CSP_ERR_SFP;
			goto error;
		}

		csp_buffer_free(packet);

	} while((packet = csp_read(conn, timeout)) != NULL);

error:
	csp_free(data);
        return error;

}
