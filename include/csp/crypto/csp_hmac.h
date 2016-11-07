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

#ifndef _CSP_HMAC_H_
#define _CSP_HMAC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#define CSP_HMAC_LENGTH	4

/**
 * Append HMAC to packet
 * @param packet Pointer to packet
 * @param include_header use header in hmac calculation (this will not modify the flags field)
 * @return 0 on success, -1 on failure
 */
int csp_hmac_append(csp_packet_t * packet, bool include_header);

/**
 * Verify HMAC of packet
 * @param packet Pointer to packet
 * @param include_header use header in hmac calculation (this will not modify the flags field)
 * @return 0 on correct HMAC, -1 if verification failed
 */
int csp_hmac_verify(csp_packet_t * packet, bool include_header);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // _CSP_HMAC_H_
