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

/* Simple implementation of XTEA in CBC mode with zero padding */

#include <stdint.h>
#include <string.h>

/* CSP includes */
#include <csp/csp.h>
#include <csp/csp_config.h>

#include "csp_xtea.h"

#if CSP_ENABLE_XTEA

#define XTEA_BLOCKSIZE 	8
#define XTEA_ROUNDS 	32

/* These functions take 64 bits of data in block[0] and block[1] and the 128 bits key in key[0] - key[3] */
static inline void xtea_encrypt_block(uint32_t block[2], uint32_t const key[4]) {

	uint32_t i, v0 = block[0], v1 = block[1], delta = 0x9E3779B9, sum = 0;
	for (i = 0; i < XTEA_ROUNDS; i++) {
		v0 += (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3]);
		sum += delta;
		v1 += (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key[(sum >> 11) & 3]);
	}
	block[0] = v0; block[1] = v1;

}

static inline void xtea_xor_byte(uint8_t * dst, uint8_t * src, uint32_t len) {

	int i;
	for (i = 0; i < len; i++)
		dst[i] ^= src[i];

}

static inline void xtea_xor_block(uint32_t * dst, uint32_t * src, uint32_t len) {

	dst[1] ^= src[1];
	dst[0] ^= src[0];

}

/* Encrypt plain text */
int xtea_encrypt(uint8_t * plain, const uint32_t len, uint32_t const key[4], uint32_t iv[2]) {

	int i;
	uint32_t stream[2];

	uint32_t blocks = (len + XTEA_BLOCKSIZE - 1)/ XTEA_BLOCKSIZE;
	uint32_t remain;

	/* Initialize stream */
	stream[0] = iv[0];
	stream[1] = iv[1];

	for (i = 0; i < blocks; i++) {
		/* Create stream */
		xtea_encrypt_block(stream, key);

		/* Calculate remaining bytes */
		remain = len - i * XTEA_BLOCKSIZE;

		/* XOR plain text with stream to generate cipher text */
		if (remain < XTEA_BLOCKSIZE)
			xtea_xor_byte(&plain[len - remain], (uint8_t *)stream, remain);
		else
			xtea_xor_byte(&plain[len - remain], (uint8_t *)stream, XTEA_BLOCKSIZE);

		/* Increment counter */
		stream[0] = iv[0];
		stream[1] = ++iv[1];
	}

	return 0;

}

/* Decrypt cipher text */
int xtea_decrypt(uint8_t * cipher, const uint32_t len, uint32_t const key[4], uint32_t iv[2]) {

	/* Since we use counter mode, we can reuse the encryption function */
	return xtea_encrypt(cipher, len, key, iv);

}

#endif // CSP_ENABLE_XTEA
