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

#ifndef _CSP_ENDIAN_H_
#define _CSP_ENDIAN_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* Convert 16-bit integers */
uint16_t csp_hton16(uint16_t h16);
uint16_t csp_ntoh16(uint16_t n16);

/* Convert 32-bit integers */
uint32_t csp_hton32(uint32_t h32);
uint32_t csp_ntoh32(uint32_t n32);

/* Convert 64-bit integers */
uint64_t csp_hton64(uint64_t h64);
uint64_t csp_ntoh64(uint64_t n64);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // _CSP_ENDIAN_H_
