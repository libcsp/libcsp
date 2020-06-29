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

/* Code originally from Python's SHA1 Module, who based it on libtom.org */

#include <csp/crypto/csp_sha1.h>

#include <string.h>

/* Rotate left macro */
#define ROL(x,y)	(((x) << (y)) | ((x) >> (32-y)))

/* Endian Neutral macros that work on all platforms */
#define STORE32H(x, y) do { (y)[0] = (uint8_t)(((x) >> 24) & 0xff); \
							(y)[1] = (uint8_t)(((x) >> 16) & 0xff); \
							(y)[2] = (uint8_t)(((x) >>  8) & 0xff); \
							(y)[3] = (uint8_t)(((x) >>  0) & 0xff); } while (0)

#define LOAD32H(x, y)  do { (x) = ((uint32_t)((y)[0] & 0xff) << 24) | \
							   	  ((uint32_t)((y)[1] & 0xff) << 16) | \
							   	  ((uint32_t)((y)[2] & 0xff) <<  8) | \
							   	  ((uint32_t)((y)[3] & 0xff) <<  0); } while (0)

#define STORE64H(x, y) do {	(y)[0] = (uint8_t)(((x) >> 56) & 0xff); \
							(y)[1] = (uint8_t)(((x) >> 48) & 0xff); \
							(y)[2] = (uint8_t)(((x) >> 40) & 0xff); \
							(y)[3] = (uint8_t)(((x) >> 32) & 0xff); \
							(y)[4] = (uint8_t)(((x) >> 24) & 0xff); \
							(y)[5] = (uint8_t)(((x) >> 16) & 0xff); \
							(y)[6] = (uint8_t)(((x) >>  8) & 0xff); \
							(y)[7] = (uint8_t)(((x) >>  0) & 0xff); } while (0)

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

/* SHA1 macros */
#define F0(x,y,z)  (z ^ (x & (y ^ z)))
#define F1(x,y,z)  (x ^ y ^ z)
#define F2(x,y,z)  ((x & y) | (z & (x | y)))
#define F3(x,y,z)  (x ^ y ^ z)

#define FF_0(a, b, c, d, e, i) do {e = (ROL(a, 5) + F0(b,c,d) + e + W[i] + 0x5a827999UL); b = ROL(b, 30);} while (0)
#define FF_1(a, b, c, d, e, i) do {e = (ROL(a, 5) + F1(b,c,d) + e + W[i] + 0x6ed9eba1UL); b = ROL(b, 30);} while (0)
#define FF_2(a, b, c, d, e, i) do {e = (ROL(a, 5) + F2(b,c,d) + e + W[i] + 0x8f1bbcdcUL); b = ROL(b, 30);} while (0)
#define FF_3(a, b, c, d, e, i) do {e = (ROL(a, 5) + F3(b,c,d) + e + W[i] + 0xca62c1d6UL); b = ROL(b, 30);} while (0)

static void csp_sha1_compress(csp_sha1_state_t * sha1, const uint8_t * buf) {

	uint32_t a, b, c, d, e, W[80], i;

	/* Copy the state into 512-bits into W[0..15] */
	for (i = 0; i < 16; i++)
		LOAD32H(W[i], buf + (4*i));

	/* Copy state */
	a = sha1->state[0];
	b = sha1->state[1];
	c = sha1->state[2];
	d = sha1->state[3];
	e = sha1->state[4];

	/* Expand it */
	for (i = 16; i < 80; i++)
		W[i] = ROL(W[i-3] ^ W[i-8] ^ W[i-14] ^ W[i-16], 1);

	/* Compress */
	i = 0;

	/* Round one */
	for (; i < 20;) {
	   FF_0(a, b, c, d, e, i++);
	   FF_0(e, a, b, c, d, i++);
	   FF_0(d, e, a, b, c, i++);
	   FF_0(c, d, e, a, b, i++);
	   FF_0(b, c, d, e, a, i++);
	}

	/* Round two */
	for (; i < 40;)  {
	   FF_1(a, b, c, d, e, i++);
	   FF_1(e, a, b, c, d, i++);
	   FF_1(d, e, a, b, c, i++);
	   FF_1(c, d, e, a, b, i++);
	   FF_1(b, c, d, e, a, i++);
	}

	/* Round three */
	for (; i < 60;)  {
	   FF_2(a, b, c, d, e, i++);
	   FF_2(e, a, b, c, d, i++);
	   FF_2(d, e, a, b, c, i++);
	   FF_2(c, d, e, a, b, i++);
	   FF_2(b, c, d, e, a, i++);
	}

	/* Round four */
	for (; i < 80;)  {
	   FF_3(a, b, c, d, e, i++);
	   FF_3(e, a, b, c, d, i++);
	   FF_3(d, e, a, b, c, i++);
	   FF_3(c, d, e, a, b, i++);
	   FF_3(b, c, d, e, a, i++);
	}

	/* Store */
	sha1->state[0] += a;
	sha1->state[1] += b;
	sha1->state[2] += c;
	sha1->state[3] += d;
	sha1->state[4] += e;

}

void csp_sha1_init(csp_sha1_state_t * sha1) {

   sha1->state[0] = 0x67452301UL;
   sha1->state[1] = 0xefcdab89UL;
   sha1->state[2] = 0x98badcfeUL;
   sha1->state[3] = 0x10325476UL;
   sha1->state[4] = 0xc3d2e1f0UL;
   sha1->curlen = 0;
   sha1->length = 0;

}

void csp_sha1_process(csp_sha1_state_t * sha1, const void * data, uint32_t inlen) {

	const uint8_t * in = data;
	uint32_t n;
	while (inlen > 0) {
		if (sha1->curlen == 0 && inlen >= CSP_SHA1_BLOCKSIZE) {
			csp_sha1_compress(sha1, in);
			sha1->length += (CSP_SHA1_BLOCKSIZE * 8);
			in += CSP_SHA1_BLOCKSIZE;
			inlen -= CSP_SHA1_BLOCKSIZE;
		} else {
			n = MIN(inlen, (CSP_SHA1_BLOCKSIZE - sha1->curlen));
			memcpy(sha1->buf + sha1->curlen, in, (size_t)n);
			sha1->curlen += n;
			in += n;
			inlen -= n;
			if (sha1->curlen == CSP_SHA1_BLOCKSIZE) {
				csp_sha1_compress(sha1, sha1->buf);
				sha1->length += (CSP_SHA1_BLOCKSIZE * 8);
				sha1->curlen = 0;
			}
		}
	}

}

void csp_sha1_done(csp_sha1_state_t * sha1, uint8_t * out) {

	uint32_t i;

	/* Increase the length of the message */
	sha1->length += sha1->curlen * 8;

	/* Append the '1' bit */
	sha1->buf[sha1->curlen++] = 0x80;

	/* If the length is currently above 56 bytes we append zeros
	 * then compress. Then we can fall back to padding zeros and length
	 * encoding like normal.
	 */
	if (sha1->curlen > 56) {
		while (sha1->curlen < 64)
			sha1->buf[sha1->curlen++] = 0;
		csp_sha1_compress(sha1, sha1->buf);
		sha1->curlen = 0;
	}

	/* Pad up to 56 bytes of zeroes */
	while (sha1->curlen < 56)
		sha1->buf[sha1->curlen++] = 0;

	/* Store length */
	STORE64H(sha1->length, sha1->buf + 56);
	csp_sha1_compress(sha1, sha1->buf);

	/* Copy output */
	for (i = 0; i < 5; i++)
		STORE32H(sha1->state[i], out + (4 * i));

}

void csp_sha1_memory(const void * msg, uint32_t len, uint8_t * hash) {

	csp_sha1_state_t md;
	csp_sha1_init(&md);
	csp_sha1_process(&md, msg, len);
	csp_sha1_done(&md, hash);

}

