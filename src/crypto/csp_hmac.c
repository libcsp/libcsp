/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2010 GomSpace ApS (gomspace.com)
Copyright (C) 2010 AAUSAT3 Project (aausat3.space.aau.dk)

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

/* Hash-based Message Authentication Code */

#include <stdint.h>
#include <string.h>

/* CSP includes */
#include <csp/csp.h>
#include <csp/csp_config.h>

#include "csp_hmac.h"
#include "csp_sha1.h"

#if CSP_ENABLE_HMAC

/**
 * Append HMAC to packet
 * @param packet Pointer to packet
 * @param key Pointer to key
 * @param klen Length of key in bytes
 * @return 0 on success, -1 on failure
 */
int hmac_append(csp_packet_t * packet, uint8_t * key, uint32_t klen) {

	if (packet == NULL || key == NULL || klen < 1)
		return -1;

	uint8_t hash[SHA1_DIGESTSIZE];

	/* Calculate H(K || M || K) */
	uint8_t * b[] = {key, packet->data, key};
	uint32_t l[] = {klen, packet->length, klen};
	sha1_hash_buffers(b, l, sizeof(l)/sizeof(l[0]), hash);

	/* Truncate hash and copy to packet */
	memcpy(&packet->data[packet->length], hash, CSP_HMAC_LENGTH);
	packet->length += CSP_HMAC_LENGTH;

	return 0;

}

/**
 * Verify HMAC of packet
 * @param packet Pointer to packet
 * @param key Pointer to key
 * @param klen Length of key in bytes
 * @return 0 on correct HMAC, -1 if verification failed
 */
int hmac_verify(csp_packet_t * packet, uint8_t * key, uint32_t klen) {

	if (packet == NULL || key == NULL || klen < 1)
		return -1;

	uint8_t hash[SHA1_DIGESTSIZE];

	/* Calculate H(K || M || K) */
	uint8_t * b[] = {key, packet->data, key};
	uint32_t l[] = {klen, packet->length - CSP_HMAC_LENGTH, klen};
	sha1_hash_buffers(b, l, sizeof(l)/sizeof(l[0]), hash);

	/* Compare calculated HMAC with packet header */
	if (memcmp(&packet->data[packet->length] - CSP_HMAC_LENGTH, hash, CSP_HMAC_LENGTH) != 0) {
		/* HMAC failed */
		return -1;
	} else {
		/* Strip HMAC */
		packet->length -= CSP_HMAC_LENGTH;
		return 0;
	}

}

#endif // CSP_ENABLE_HMAC
