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

#include <stdint.h>
#include <stdlib.h>
#include <csp/csp.h>
#include <csp/csp_endian.h>
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
static sfp_header_t * csp_sfp_header_add(csp_packet_t * packet) {
	sfp_header_t * header = (sfp_header_t *) &packet->data[packet->length];
	packet->length += sizeof(sfp_header_t);
	memset(header, 0, sizeof(sfp_header_t));
	return header;
}

static sfp_header_t * csp_sfp_header_remove(csp_packet_t * packet) {
	sfp_header_t * header = (sfp_header_t *) &packet->data[packet->length-sizeof(sfp_header_t)];
	packet->length -= sizeof(sfp_header_t);
	return header;
}

int csp_sfp_send(csp_conn_t * conn, void * data, int totalsize, int mtu, uint32_t timeout) {

	int count = 0;
	while(count < totalsize) {

		/* Allocate packet */
		csp_packet_t * packet = csp_buffer_get(mtu);
		if (packet == NULL)
			return -1;

		/* Calculate sending size */
		int size = totalsize - count;
		if (size > mtu)
			size = mtu;

		/* Print debug */
		csp_debug(CSP_PROTOCOL, "Sending SFP at %x size %u\r\n", data + count, size);

		/* Copy data */
		memcpy(packet->data, data + count, size);
		packet->length = size;

		/* Set fragment flag */
		conn->idout.flags |= CSP_FFRAG;

		/* Add SFP header */
		sfp_header_t * sfp_header = csp_sfp_header_add(packet);
		sfp_header->totalsize = csp_hton32(totalsize);
		sfp_header->offset = csp_hton32(count);

		/* Send data */
		if (!csp_send(conn, packet, timeout)) {
			csp_buffer_free(packet);
			return -1;
		}

		/* Increment count */
		count += size;

	}

	return 0;

}

int csp_sfp_recv(csp_conn_t * conn, void ** dataout, int * datasize, uint32_t timeout) {

	unsigned int last_byte = 0;

	csp_packet_t * packet;
	while((packet = csp_read(conn, timeout)) != NULL) {

		/* Check that SFP header is present */
		if ((packet->id.flags & CSP_FFRAG) == 0) {
			csp_debug(CSP_ERROR, "Missing SFP header\r\n");
			return -1;
		}

		/* Read SFP header */
		sfp_header_t * sfp_header = csp_sfp_header_remove(packet);
		sfp_header->offset = csp_ntoh32(sfp_header->offset);
		sfp_header->totalsize = csp_ntoh32(sfp_header->totalsize);

		csp_debug(CSP_PROTOCOL, "SFP fragment %u/%u\r\n", sfp_header->offset + packet->length, sfp_header->totalsize);

		if (sfp_header->offset > last_byte + 1) {
			csp_debug(CSP_ERROR, "SFP missing %u bytes\r\n", sfp_header->offset - last_byte);
			csp_buffer_free(packet);
			return -1;
		} else {
			last_byte = sfp_header->offset + packet->length;
		}

		/* Allocate memory */
		if (*dataout == NULL)
			*dataout = malloc(sfp_header->totalsize);
		*datasize = sfp_header->totalsize;

		/* Copy data to output */
		if (*dataout != NULL)
			memcpy(*dataout + sfp_header->offset, packet->data, packet->length);

		if (sfp_header->offset + packet->length >= sfp_header->totalsize) {
			csp_debug(CSP_PROTOCOL, "SFP complete\r\n");
			csp_buffer_free(packet);
			return 0;
		} else {
			csp_buffer_free(packet);
		}

	}

	return -1;

}

