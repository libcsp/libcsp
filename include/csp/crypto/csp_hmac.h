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

#ifndef _CSP_CRYPTO_HMAC_H_
#define _CSP_CRYPTO_HMAC_H_

/**
   @file
   HMAC support.

   Hash-based Message Authentication Code - based on code from libtom.org.
*/

#include <csp/crypto/csp_sha1.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
   Number of bytes from the HMAC calculation, that is appended to the CSP message.
*/
#define CSP_HMAC_LENGTH	4

/**
 * Append HMAC to packet
 * @param packet CSP packet, must be valid.
 * @param include_header use header in hmac calculation (this will not modify the flags field)
 * @return #CSP_ERR_NONE on success, otherwise an error code.
 */
int csp_hmac_append(csp_packet_t * packet, bool include_header);

/**
 * Verify HMAC of packet
 * @param packet CSP packet, must be valid.
 * @param include_header use header in hmac calculation (this will not modify the flags field)
 * @return #CSP_ERR_NONE on success, otherwise an error code.
 */
int csp_hmac_verify(csp_packet_t * packet, bool include_header);

/**
 * Calculate HMAC on buffer
 *
 * This function is used by append/verify but cal also be called separately.
 * @param key HMAC key
 * @param keylen HMAC key length
 * @param data pointer to data
 * @param datalen lehgth of data
 * @param[out] hmac calculated HMAC hash, minimum #CSP_SHA1_DIGESTSIZE bytes.
 * @return #CSP_ERR_NONE on success, otherwise an error code.
 */
int csp_hmac_memory(const void * key, uint32_t keylen, const void * data, uint32_t datalen, uint8_t * hmac);

/**
 * Save a copy of the key string for use by the append/verify functions
 * @param key HMAC key
 * @param keylen HMAC key length
 * @return #CSP_ERR_NONE on success, otherwise an error code.
 */
int csp_hmac_set_key(const void * key, uint32_t keylen);

#ifdef __cplusplus
}
#endif
#endif
