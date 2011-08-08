/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2011 GomSpace ApS (http://www.gomspace.com)
Copyright (C) 2011 AAUSAT3 Project (http://aausat3.space.aau.dk)

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
#include <string.h>
#include <inttypes.h>

#include <csp/csp.h>

#if CSP_ENABLE_CRC32

#define CSP_CRC32_POLY 0x82F63B78L

uint32_t crc_tab[256];

void csp_crc32_gentab(void) {
	uint32_t crc;
	int i, j;

	for (i = 0; i < 256; i++) {
		crc = i;
		for (j = 8; j > 0; j--) {
			if (crc & 1)
				crc = (crc >> 1) ^ CSP_CRC32_POLY;
			else
				crc >>= 1;
		}
		crc_tab[i] = crc;
	}
}

uint32_t csp_crc32_memory(const uint8_t * data, uint32_t length) {
   uint32_t crc;

   crc = 0xFFFFFFFF;
   while (length--)
	   crc = crc_tab[(crc ^ *data++) & 0xFFL] ^ (crc >> 8);

   return (crc ^ 0xFFFFFFFF);
}

int csp_crc32_append(csp_packet_t * packet) {

	uint32_t crc;

	/* NULL pointer check */
	if (packet == NULL)
		return -1;

	/* Calculate CRC32 */
	crc = csp_crc32_memory(packet->data, packet->length);

	/* Truncate hash and copy to packet */
	memcpy(&packet->data[packet->length], &crc, sizeof(uint32_t));
	packet->length += sizeof(uint32_t);

	return 0;

}

int csp_crc32_verify(csp_packet_t * packet) {

	uint32_t crc;

	/* NULL pointer check */
	if (packet == NULL)
		return -1;

	/* Calculate CRC32 */
	crc = csp_crc32_memory(packet->data, packet->length - sizeof(uint32_t));

	/* Compare calculated HMAC with packet header */
	if (memcmp(&packet->data[packet->length] - sizeof(uint32_t), &crc, sizeof(uint32_t)) != 0) {
		/* CRC32 failed */
		return -1;
	} else {
		/* Strip CRC32 */
		packet->length -= sizeof(uint32_t);
		return 0;
	}

}

#endif // CSP_ENABLE_CRC32
