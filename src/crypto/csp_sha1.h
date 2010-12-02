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
} sha1_state;

void sha1_init(sha1_state * sha1);
void sha1_process(sha1_state * sha1, const uint8_t * in, uint32_t inlen);
void sha1_done(sha1_state * sha1, uint8_t * out);
void sha1_memory(const uint8_t * msg, uint32_t len, uint8_t * hash);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // _CSP_SHA1_H_
