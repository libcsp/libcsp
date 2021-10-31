

#pragma once

/**
   @file
   XTEA support.
*/

#include <csp/csp_types.h>



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
