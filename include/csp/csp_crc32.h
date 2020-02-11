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

#ifndef _CSP_CRC32_H_
#define _CSP_CRC32_H_

/**
   @file
   CRC32 support.
*/

#include <csp/csp.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
   Append CRC32 checksum to packet
   @param[in] packet CSP packet, must be valid.
   @param[in] include_header include the CSP header in the CRC32, otherwise just the data part.
   @return #CSP_ERR_NONE on success, otherwise an error code.
*/
int csp_crc32_append(csp_packet_t * packet, bool include_header);

/**
   Verify CRC32 checksum on packet.
   @param[in] packet CSP packet, must be valid.
   @param[in] include_header include the CSP header in the CRC32, otherwise just the data part.
   @return #CSP_ERR_NONE on success, otherwise an error code.
*/
int csp_crc32_verify(csp_packet_t * packet, bool include_header);

/**
   Calculate checksum for a given memory area.
   @param[in] addr memory address
   @param[in] length length of memory to do checksum on
   @return checksum
*/
uint32_t csp_crc32_memory(const uint8_t * addr, uint32_t length);

#ifdef __cplusplus
}
#endif
#endif
