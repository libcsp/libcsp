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

#ifndef _CSP_CRYPTO_XTEA_H_
#define _CSP_CRYPTO_XTEA_H_

/**
   @file
   XTEA support.
*/

#include <csp/csp_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
   Set XTEA key
   @param[in] key XTEA key
   @param[in] keylen length of key
   @return #CSP_ERR_NONE on success, otherwise an error code.
*/
int csp_xtea_set_key(const void * key, uint32_t keylen);

/**
   XTEA encrypt byte array
   @param[in] data data to be encrypted.
   @param[in] len Length of \a data.
   @param[in] iv Initialization vector
   @return #CSP_ERR_NONE on success, otherwise an error code.
*/
int csp_xtea_encrypt(void * data, uint32_t len, uint32_t iv[2]);

/**
   XTEA encrypt message.
   @param packet CSP packet, must be valid.
   @return #CSP_ERR_NONE on success, otherwise an error code.
*/
int csp_xtea_encrypt_packet(csp_packet_t * packet);

/**
   Decrypt XTEA encrypted byte array.
   @param[in,out] encrypted data be decrypted.
   @param[in] len length of \a encrypted data.
   @param[in] iv Initialization vector.
   @return #CSP_ERR_NONE on success, otherwise an error code.
*/
int csp_xtea_decrypt(void * encrypted, uint32_t len, uint32_t iv[2]);

/**
   XTEA decrypt message.
   @param packet CSP packet, must be valid.
   @return #CSP_ERR_NONE on success, otherwise an error code.
*/
int csp_xtea_decrypt_packet(csp_packet_t * packet);
    
#ifdef __cplusplus
}
#endif
#endif
