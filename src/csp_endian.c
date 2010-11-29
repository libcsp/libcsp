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

#include <stdint.h>

/* CSP includes */
#include <csp/csp_endian.h>

/*
 * These functions are declared __const__, so the compiler
 * should identify their return value as a runtime constant
 */

/* Returns 1 on big endian archs and 0 on little endian */
static inline int __attribute__ ((__const__)) host_be(void) {
    static union {
        uint32_t i;
        uint8_t c[4];
    } un = {0x11223344};

    return un.c[0] == 0x11;
} 

/* Convert 16-bit integer to network byte order */
inline uint16_t __attribute__ ((__const__)) csp_hton16(uint16_t h16) {
	if (host_be()) {
		return h16;
	} else {
		return (((h16 & 0xff00) >> 8) |
				((h16 & 0x00ff) << 8));
	}
}

/* Convert 16-bit integer to host byte order */
inline uint16_t __attribute__ ((__const__)) csp_ntoh16(uint16_t n16) {
	return csp_hton16(n16);
}

/* Convert 32-bit integer to network byte order */
inline uint32_t __attribute__ ((__const__)) csp_hton32(uint32_t h32) {
	if (host_be()) {
		return h32;
	} else {
		return (((h32 & 0xff000000) >> 24) |
				((h32 & 0x000000ff) << 24) |
				((h32 & 0x0000ff00) <<  8) |
				((h32 & 0x00ff0000) >>  8));
	}
}

/* Convert 32-bit integer to host byte order */
inline uint32_t __attribute__ ((__const__)) csp_ntoh32(uint32_t n32) {
	return csp_hton32(n32);
}

/* Convert 64-bit integer to network byte order */
inline uint64_t __attribute__ ((__const__)) csp_hton64(uint64_t h64) {
	if (host_be()) {
		return h64;
	} else {
		return (((h64 & 0xff00000000000000) >> 56) |
				((h64 & 0x00000000000000ff) << 56) |
				((h64 & 0x00ff000000000000) >> 40) |
				((h64 & 0x000000000000ff00) << 40) |
				((h64 & 0x0000ff0000000000) >> 24) |
				((h64 & 0x0000000000ff0000) << 24) |
				((h64 & 0x000000ff00000000) >>  8) |
				((h64 & 0x00000000ff000000) <<  8));
	}
}

/* Convert 64-bit integer to host byte order */
inline uint64_t __attribute__ ((__const__)) csp_ntoh64(uint64_t n64) {
	return csp_hton64(n64);
}

/* Compatibility functions */

/* Convert 16-bit integer to network byte order */
inline uint16_t __attribute__ ((__const__)) htons(uint16_t hs) {
    return csp_hton16(hs);
}

/* Convert 16-bit integer to host byte order */
inline uint16_t __attribute__ ((__const__)) ntohs(uint16_t ns) {
    return csp_ntoh16(ns);
}

/* Convert 32-bit integer to network byte order */
inline uint32_t __attribute__ ((__const__)) htonl(uint32_t hl) {
    return csp_hton32(hl);
}

/* Convert 32-bit integer to host byte order */
inline uint32_t __attribute__ ((__const__)) ntohl(uint32_t nl) {
    return csp_ntoh32(nl);
}
