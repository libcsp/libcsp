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

/**
   @file
   Endian support.
*/

#include <csp/csp_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
   Convert from host order to network order.
*/
uint16_t csp_hton16(uint16_t h16);

/**
   Convert from network order to host order.
*/
uint16_t csp_ntoh16(uint16_t n16);

/**
   Convert from host order to network order.
*/
uint32_t csp_hton32(uint32_t h32);

/**
   Convert from network order to host order.
*/
uint32_t csp_ntoh32(uint32_t n32);

/**
   Convert from host order to network order.
*/
uint64_t csp_hton64(uint64_t h64);

/**
   Convert from network order to host order.
*/
uint64_t csp_ntoh64(uint64_t n64);

/**
   Convert from host order to big endian.
*/
uint16_t csp_htobe16(uint16_t h16);

/**
   Convert from host order to little endian.
*/
uint16_t csp_htole16(uint16_t h16);

/**
   Convert from big endian to host order.
*/
uint16_t csp_betoh16(uint16_t be16);

/**
   Convert from little endian to host order.
*/
uint16_t csp_letoh16(uint16_t le16);

/**
   Convert from host order to big endian.
 */
uint32_t csp_htobe32(uint32_t h32);

/**
   Convert from host order to little endian.
*/
uint32_t csp_htole32(uint32_t h32);

/**
   Convert from big endian to host order.
*/
uint32_t csp_betoh32(uint32_t be32);

/**
   Convert from little endian to host order.
*/
uint32_t csp_letoh32(uint32_t le32);

/**
   Convert from host order to big endian.
*/
uint64_t csp_htobe64(uint64_t h64);

/**
   Convert from host order to little endian.
*/
uint64_t csp_htole64(uint64_t h64);

/**
   Convert from big endian to host order.
*/
uint64_t csp_betoh64(uint64_t be64);

/**
   Convert from little endian to host order.
*/
uint64_t csp_letoh64(uint64_t le64);

/**
   Convert from host order to network order.
*/
float csp_htonflt(float f);

/**
   Convert from network order to host order.
*/
float csp_ntohflt(float f);

/**
   Convert from host order to network order.
*/
double csp_htondbl(double d);

/**
   Convert from network order to host order.
*/
double csp_ntohdbl(double d);

#ifdef __cplusplus
}
#endif
#endif // _CSP_ENDIAN_H_
