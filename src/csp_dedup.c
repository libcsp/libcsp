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

#include "csp_dedup.h"

#include <stdlib.h>

#include <csp/arch/csp_time.h>
#include <csp/csp_crc32.h>

/* Check the last CSP_DEDUP_COUNT packets for duplicates */
#define CSP_DEDUP_COUNT		16

/* Only consider packet a duplicate if received under CSP_DEDUP_WINDOW_MS ago */
#define CSP_DEDUP_WINDOW_MS	1000

/* Store packet CRC's in a ringbuffer */
static uint32_t csp_dedup_array[CSP_DEDUP_COUNT] = {};
static uint32_t csp_dedup_timestamp[CSP_DEDUP_COUNT] = {};
static int csp_dedup_in = 0;

bool csp_dedup_is_duplicate(csp_packet_t *packet)
{
	/* Calculate CRC32 for packet */
	uint32_t crc = csp_crc32_memory((const uint8_t *) &packet->id, packet->length + sizeof(packet->id));

	/* Check if we have received this packet before */
	for (int i = 0; i < CSP_DEDUP_COUNT; i++) {

		/* Check for match */
		if (crc == csp_dedup_array[i]) {

			/* Check the timestamp */
			if (csp_get_ms() < csp_dedup_timestamp[i] + CSP_DEDUP_WINDOW_MS)
				return true;
		}
	}

	/* If not, insert packet into duplicate list */
	csp_dedup_array[csp_dedup_in] = crc;
	csp_dedup_timestamp[csp_dedup_in] = csp_get_ms();
	csp_dedup_in = (csp_dedup_in + 1) % CSP_DEDUP_COUNT;

	return false;
}
