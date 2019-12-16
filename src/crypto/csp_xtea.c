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

/* Simple implementation of XTEA in CTR mode */

#include <csp/crypto/csp_xtea.h>

#include <string.h>
#include <stdlib.h>

#include <csp/csp_endian.h>
#include <csp/csp_buffer.h>
#include <csp/crypto/csp_sha1.h>

#define XTEA_BLOCKSIZE 	8
#define XTEA_ROUNDS 	32
#define XTEA_KEY_LENGTH	16

/* XTEA key */
static uint32_t csp_xtea_key[XTEA_KEY_LENGTH/sizeof(uint32_t)] __attribute__ ((aligned(sizeof(uint32_t))));

#define STORE32L(x, y) do { (y)[3] = (uint8_t)(((x) >> 24) & 0xff); \
							(y)[2] = (uint8_t)(((x) >> 16) & 0xff); \
							(y)[1] = (uint8_t)(((x) >> 8) & 0xff); \
							(y)[0] = (uint8_t)(((x) >> 0) & 0xff); } while (0)

#define LOAD32L(x, y) do { (x) = ((uint32_t)((y)[3] & 0xff) << 24) | \
								 ((uint32_t)((y)[2] & 0xff) << 16) | \
								 ((uint32_t)((y)[1] & 0xff) << 8)  | \
								 ((uint32_t)((y)[0] & 0xff) << 0); } while (0)

/* This function takes 64 bits of data in block and the 128 bits key in key */
static inline void csp_xtea_encrypt_block(uint8_t *block, uint8_t const *key) {

	uint32_t i, v0, v1, delta = 0x9E3779B9, sum = 0, k[4];

	LOAD32L(k[0], &key[0]);
	LOAD32L(k[1], &key[4]);
	LOAD32L(k[2], &key[8]);
	LOAD32L(k[3], &key[12]);

	LOAD32L(v0, &block[0]);
	LOAD32L(v1, &block[4]);

	for (i = 0; i < XTEA_ROUNDS; i++) {
		v0 += (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + k[sum & 3]);
		sum += delta;
		v1 += (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + k[(sum >> 11) & 3]);
	}

	STORE32L(v0, &block[0]);
	STORE32L(v1, &block[4]);

}

static inline void csp_xtea_xor_byte(uint8_t * dst, uint8_t * src, uint32_t len) {

	unsigned int i;
	for (i = 0; i < len; i++)
		dst[i] ^= src[i];

}

int csp_xtea_set_key(const void * key, uint32_t keylen) {

	/* Use SHA1 as KDF */
	uint8_t hash[CSP_SHA1_DIGESTSIZE];
	csp_sha1_memory(key, keylen, hash);

	/* Copy key */
	memcpy(csp_xtea_key, hash, XTEA_KEY_LENGTH);

	return CSP_ERR_NONE;

}

int csp_xtea_encrypt(void * plain, const uint32_t len, uint32_t iv[2]) {

	unsigned int i;
	uint32_t stream[2];

	uint32_t blocks = (len + XTEA_BLOCKSIZE - 1)/ XTEA_BLOCKSIZE;
	uint32_t remain;

	/* Initialize stream */
	stream[0] = csp_htobe32(iv[0]);
	stream[1] = csp_htobe32(iv[1]);

	for (i = 0; i < blocks; i++) {
		/* Create stream */
		csp_xtea_encrypt_block((uint8_t *)stream, (uint8_t *)csp_xtea_key);

		/* Calculate remaining bytes */
		remain = len - i * XTEA_BLOCKSIZE;

		/* XOR plain text with stream to generate cipher text */
		csp_xtea_xor_byte(&((uint8_t*)plain)[len - remain], (uint8_t *)stream, remain < XTEA_BLOCKSIZE ? remain : XTEA_BLOCKSIZE);

		/* Increment counter */
		stream[0] = csp_htobe32(iv[0]);
		stream[1] = csp_htobe32(iv[1]++);
	}

	return CSP_ERR_NONE;

}

int csp_xtea_encrypt_packet(csp_packet_t * packet) {

	/* Create nonce */
	const uint32_t nonce = (uint32_t)rand();
	const uint32_t nonce_n = csp_hton32(nonce);

	if ((packet->length + sizeof(nonce_n)) > csp_buffer_data_size()) {
		return CSP_ERR_NOMEM;
	}

	/* Create initialization vector */
	uint32_t iv[2] = {nonce, 1};

	/* Encrypt data */
	if (csp_xtea_encrypt(packet->data, packet->length, iv) != CSP_ERR_NONE) {
		return CSP_ERR_XTEA;
        }

	memcpy(&packet->data[packet->length], &nonce_n, sizeof(nonce_n));
	packet->length += sizeof(nonce_n);

	return CSP_ERR_NONE;

}

int csp_xtea_decrypt(void * cipher, const uint32_t len, uint32_t iv[2]) {

	/* Since we use counter mode, we can reuse the encryption function */
	return csp_xtea_encrypt(cipher, len, iv);

}

int csp_xtea_decrypt_packet(csp_packet_t * packet) {

	/* Read nonce */
	uint32_t nonce;

	if (packet->length < sizeof(nonce)) {
		return CSP_ERR_XTEA;
	}

        memcpy(&nonce, &packet->data[packet->length - sizeof(nonce)], sizeof(nonce));
	nonce = csp_ntoh32(nonce);

	/* Create initialization vector */
	uint32_t iv[2] = {nonce, 1};

	/* Decrypt data */
	if (csp_xtea_decrypt(packet->data, packet->length, iv) != CSP_ERR_NONE) {
		return CSP_ERR_XTEA;
	}

	packet->length -= sizeof(nonce);

	return CSP_ERR_NONE;

}
