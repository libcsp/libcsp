/****************************************************************************
 * **File:** csp/crypto/csp_hmac.h
 *
 * **Description:** HMAC support.
 *
 * Hash-based Message Authentication Code - based on code from libtom.org.
 ****************************************************************************/
#pragma once

#include <csp/crypto/csp_sha1.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Number of bytes from the HMAC calculation, that is appended to the CSP message.
 */
#define CSP_HMAC_LENGTH	4

/**
 * Append HMAC to packet
 * If header is included, csp_id_prepend() must be called beforehand
 *
 * @param[in] packet CSP packet, must be valid.
 * @param[in] include_header use header in hmac calculation (this will not modify the flags field)
 * @return #CSP_ERR_NONE on success, otherwise an error code.
 */
int csp_hmac_append(csp_packet_t * packet, bool include_header);

/**
 * Verify HMAC of packet
 *
 * @param[in] packet CSP packet, must be valid.
 * @param[in] include_header use header in hmac calculation (this will not modify the flags field)
 * @return #CSP_ERR_NONE on success, otherwise an error code.
 */
int csp_hmac_verify(csp_packet_t * packet, bool include_header);

/**
 * Calculate HMAC on buffer
 * This function is used by append/verify but cal also be called separately.
 *
 * @param[in] key HMAC key
 * @param[in] keylen HMAC key length
 * @param[in] data pointer to data
 * @param[in] datalen lehgth of data
 * @param[out] hmac calculated HMAC hash, minimum #CSP_SHA1_DIGESTSIZE bytes.
 * @return #CSP_ERR_NONE on success, otherwise an error code.
 */
int csp_hmac_memory(const void * key, uint32_t keylen, const void * data, uint32_t datalen, uint8_t * hmac);

/**
 * Save a copy of the key string for use by the append/verify functions
 *
 * @param[in] key HMAC key
 * @param[in] keylen HMAC key length
 * @return #CSP_ERR_NONE on success, otherwise an error code.
 */
int csp_hmac_set_key(const void * key, uint32_t keylen);

#ifdef __cplusplus
}
#endif
