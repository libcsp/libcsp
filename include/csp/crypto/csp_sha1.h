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

#ifndef _CSP_SHA1_H_
#define _CSP_SHA1_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* The SHA1 block and message digest size in bytes */
#define SHA1_BLOCKSIZE	64
#define SHA1_DIGESTSIZE	20

/* SHA1 state structure */
typedef struct {
	uint64_t length;
	uint32_t state[5], curlen;
	uint8_t buf[SHA1_BLOCKSIZE];
} csp_sha1_state;

/**
 * Initialize the hash state
 * @param sha1   The hash state you wish to initialize
 */
void csp_sha1_init(csp_sha1_state * sha1);

/**
 * Process a block of memory though the hash
 * @param sha1   The hash state
 * @param in	 The data to hash
 * @param inlen  The length of the data (octets)
 */
void csp_sha1_process(csp_sha1_state * sha1, const uint8_t * in, uint32_t inlen);

/**
 * Terminate the hash to get the digest
 * @param sha1  The hash state
 * @param out [out] The destination of the hash (20 bytes)
 */
void csp_sha1_done(csp_sha1_state * sha1, uint8_t * out);

/**
 * Calculate SHA1 hash of block of memory.
 * @param msg   Pointer to message buffer
 * @param len   Length of message
 * @param sha1  Pointer to SHA1 output buffer. Must be 20 bytes or more!
 */
void csp_sha1_memory(const uint8_t * msg, uint32_t len, uint8_t * hash);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // _CSP_SHA1_H_
