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

#include <csp/csp_endian.h>

/* Convert 16-bit number from host byte order to network byte order */
inline uint16_t __attribute__ ((__const__)) csp_hton16(uint16_t h16) {
#if (CSP_BIG_ENDIAN)
	return h16;
#else
	return (((h16 & 0xff00) >> 8) |
			((h16 & 0x00ff) << 8));
#endif
}

/* Convert 16-bit number from network byte order to host byte order */
inline uint16_t __attribute__ ((__const__)) csp_ntoh16(uint16_t n16) {
	return csp_hton16(n16);
}

/* Convert 32-bit number from host byte order to network byte order */
inline uint32_t __attribute__ ((__const__)) csp_hton32(uint32_t h32) {
#if (CSP_BIG_ENDIAN)
	return h32;
#else
	return (((h32 & 0xff000000) >> 24) |
			((h32 & 0x000000ff) << 24) |
			((h32 & 0x0000ff00) <<  8) |
			((h32 & 0x00ff0000) >>  8));
#endif
}

/* Convert 32-bit number from network byte order to host byte order */
inline uint32_t __attribute__ ((__const__)) csp_ntoh32(uint32_t n32) {
	return csp_hton32(n32);
}

/* Convert 64-bit number from host byte order to network byte order */
inline uint64_t __attribute__ ((__const__)) csp_hton64(uint64_t h64) {
#if (CSP_BIG_ENDIAN)
	return h64;
#else
	return (((h64 & 0xff00000000000000LL) >> 56) |
			((h64 & 0x00000000000000ffLL) << 56) |
			((h64 & 0x00ff000000000000LL) >> 40) |
			((h64 & 0x000000000000ff00LL) << 40) |
			((h64 & 0x0000ff0000000000LL) >> 24) |
			((h64 & 0x0000000000ff0000LL) << 24) |
			((h64 & 0x000000ff00000000LL) >>  8) |
			((h64 & 0x00000000ff000000LL) <<  8));
#endif
}

/* Convert 64-bit number from host byte order to network byte order */
inline uint64_t __attribute__ ((__const__)) csp_ntoh64(uint64_t n64) {
	return csp_hton64(n64);
}

/* Convert 16-bit number from host byte order to big endian byte order */
inline uint16_t __attribute__ ((__const__)) csp_htobe16(uint16_t h16) {
	return csp_hton16(h16);
}

/* Convert 16-bit number from host byte order to little endian byte order */
inline uint16_t __attribute__ ((__const__)) csp_htole16(uint16_t h16) {
#if (CSP_LITTLE_ENDIAN)
	return h16;
#else
	return (((h16 & 0xff00) >> 8) |
			((h16 & 0x00ff) << 8));
#endif
}

/* Convert 16-bit number from big endian byte order to little endian byte order */
inline uint16_t __attribute__ ((__const__)) csp_betoh16(uint16_t be16) {
	return csp_ntoh16(be16);
}

/* Convert 16-bit number from little endian byte order to host byte order */
inline uint16_t __attribute__ ((__const__)) csp_letoh16(uint16_t le16) {
	return csp_htole16(le16);
}

/* Convert 32-bit number from host byte order to big endian byte order */
inline uint32_t __attribute__ ((__const__)) csp_htobe32(uint32_t h32) {
	return csp_hton32(h32);
}

/* Convert 32-bit number from little endian byte order to host byte order */
inline uint32_t __attribute__ ((__const__)) csp_htole32(uint32_t h32) {
#if (CSP_LITTLE_ENDIAN)
	return h32;
#else
	return (((h32 & 0xff000000) >> 24) |
			((h32 & 0x000000ff) << 24) |
			((h32 & 0x0000ff00) <<  8) |
			((h32 & 0x00ff0000) >>  8));
#endif
}

/* Convert 32-bit number from big endian byte order to host byte order */
inline uint32_t __attribute__ ((__const__)) csp_betoh32(uint32_t be32) {
	return csp_ntoh32(be32);
}

/* Convert 32-bit number from little endian byte order to host byte order */
inline uint32_t __attribute__ ((__const__)) csp_letoh32(uint32_t le32) {
	return csp_htole32(le32);
}

/* Convert 64-bit number from host byte order to big endian byte order */
inline uint64_t __attribute__ ((__const__)) csp_htobe64(uint64_t h64) {
	return csp_hton64(h64);
}

/* Convert 64-bit number from host byte order to little endian byte order */
inline uint64_t __attribute__ ((__const__)) csp_htole64(uint64_t h64) {
#if (CSP_LITTLE_ENDIAN)
	return h64;
#else
	return (((h64 & 0xff00000000000000LL) >> 56) |
			((h64 & 0x00000000000000ffLL) << 56) |
			((h64 & 0x00ff000000000000LL) >> 40) |
			((h64 & 0x000000000000ff00LL) << 40) |
			((h64 & 0x0000ff0000000000LL) >> 24) |
			((h64 & 0x0000000000ff0000LL) << 24) |
			((h64 & 0x000000ff00000000LL) >>  8) |
			((h64 & 0x00000000ff000000LL) <<  8));
#endif
}

/* Convert 64-bit number from big endian byte order to host byte order */
inline uint64_t __attribute__ ((__const__)) csp_betoh64(uint64_t be64) {
	return csp_ntoh64(be64);
}

/* Convert 64-bit number from little endian byte order to host byte order */
inline uint64_t __attribute__ ((__const__)) csp_letoh64(uint64_t le64) {
	return csp_htole64(le64);
}


/* Convert float from host byte order to network byte order */
inline float __attribute__ ((__const__)) csp_htonflt(float f) {
#if (CSP_BIG_ENDIAN)
	return f;
#else
	union v {
		float       f;
		uint32_t	i;
	};
	union v val;
	val.f = f;
	val.i = csp_hton32(val.i);
	return val.f;
#endif
}

/* Convert float from host byte order to network byte order */
inline float __attribute__ ((__const__)) csp_ntohflt(float f) {
	return csp_htonflt(f);
}

/* Convert double from host byte order to network byte order */
inline double __attribute__ ((__const__)) csp_htondbl(double d) {
#if (CSP_BIG_ENDIAN)
	return d;
#else
	union v {
		double       d;
		uint64_t     i;
	};
	union v val;
	val.d = d;
	val.i = csp_hton64(val.i);
	return val.d;
#endif
}

/* Convert float from host byte order to network byte order */
inline double __attribute__ ((__const__)) csp_ntohdbl(double d) {
	return csp_htondbl(d);
}
