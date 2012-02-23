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

#ifndef _CSP_ENDIAN_H_
#define _CSP_ENDIAN_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * Convert 16-bit integer from host byte order to network byte order
 * @param h16 Host byte order 16-bit integer
 */
uint16_t csp_hton16(uint16_t h16);

/**
 * Convert 16-bit integer from host byte order to host byte order
 * @param n16 Network byte order 16-bit integer
 */
uint16_t csp_ntoh16(uint16_t n16);

/**
 * Convert 32-bit integer from host byte order to network byte order
 * @param h32 Host byte order 32-bit integer
 */
uint32_t csp_hton32(uint32_t h32);

/**
 * Convert 32-bit integer from host byte order to host byte order
 * @param n32 Network byte order 32-bit integer
 */
uint32_t csp_ntoh32(uint32_t n32);

/**
 * Convert 64-bit integer from host byte order to network byte order
 * @param h64 Host byte order 64-bit integer
 */
uint64_t csp_hton64(uint64_t h64);

/**
 * Convert 64-bit integer from host byte order to host byte order
 * @param n64 Network byte order 64-bit integer
 */
uint64_t csp_ntoh64(uint64_t n64);

/**
 * Convert 16-bit integer from host byte order to big endian byte order
 * @param h16 Host byte order 16-bit integer
 */
uint16_t csp_htobe16(uint16_t h16);

/**
 * Convert 16-bit integer from host byte order to little endian byte order
 * @param h16 Host byte order 16-bit integer
 */
uint16_t csp_htole16(uint16_t h16);

/**
 * Convert 16-bit integer from big endian byte order to little endian byte order
 * @param be16 Big endian byte order 16-bit integer
 */
uint16_t csp_betoh16(uint16_t be16);

/**
 * Convert 16-bit integer from little endian byte order to host byte order
 * @param le16 Little endian byte order 16-bit integer
 */
uint16_t csp_letoh16(uint16_t le16);

/**
 * Convert 32-bit integer from host byte order to big endian byte order
 * @param h32 Host byte order 32-bit integer
 */
uint32_t csp_htobe32(uint32_t h32);

/**
 * Convert 32-bit integer from little endian byte order to host byte order
 * @param h32 Host byte order 32-bit integer
 */
uint32_t csp_htole32(uint32_t h32);

/**
 * Convert 32-bit integer from big endian byte order to host byte order
 * @param be32 Big endian byte order 32-bit integer
 */
uint32_t csp_betoh32(uint32_t be32);

/**
 * Convert 32-bit integer from little endian byte order to host byte order
 * @param le32 Little endian byte order 32-bit integer
 */
uint32_t csp_letoh32(uint32_t le32);

/**
 * Convert 64-bit integer from host byte order to big endian byte order
 * @param h64 Host byte order 64-bit integer
 */
uint64_t csp_htobe64(uint64_t h64);

/**
 * Convert 64-bit integer from host byte order to little endian byte order
 * @param h64 Host byte order 64-bit integer
 */
uint64_t csp_htole64(uint64_t h64);

/**
 * Convert 64-bit integer from big endian byte order to host byte order
 * @param be64 Big endian byte order 64-bit integer
 */
uint64_t csp_betoh64(uint64_t be64);

/**
 * Convert 64-bit integer from little endian byte order to host byte order
 * @param le64 Little endian byte order 64-bit integer
 */
uint64_t csp_letoh64(uint64_t le64);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // _CSP_ENDIAN_H_
