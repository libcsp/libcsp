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

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <inttypes.h>

#include <csp/csp.h>
#include <csp/csp_crc32.h>

#define CSP_DEDUP_BITMASK	0x7 //! Defines how many packet CRC that csp_dedup should use

/* Store packet CRC's in a ringbuffer */
static uint32_t csp_dedup_array[CSP_DEDUP_BITMASK] = {};
static int csp_dedup_in = 0;

int csp_dedup_check(csp_packet_t * packet) {

	/* Calculate CRC32 for packet */
	uint32_t crc = csp_crc32_memory((const uint8_t *) &packet->id, packet->length + sizeof(packet->id));

	/* Check if we have received this packet before,
	 * Use a bit obscure but fast method of looping backwards in the array with overflow using two's compliment method */
	for (int i = (csp_dedup_in - 1) & CSP_DEDUP_BITMASK; i != csp_dedup_in; i = (i - 1) & CSP_DEDUP_BITMASK) {
		if (crc == csp_dedup_array[i])
			return 1;
	}

	/* If not, insert CRC into memory */
	csp_dedup_array[csp_dedup_in] = crc;
	csp_dedup_in = (csp_dedup_in + 1) & CSP_DEDUP_BITMASK;

	return 0;
}
