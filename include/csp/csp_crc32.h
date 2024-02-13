/****************************************************************************
 * **File:** csp/csp_crc32.h
 *
 * **Description:** CRC32 support.
 ****************************************************************************/
#pragma once

#include <csp/csp.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
   CRC32 calculation object (digest object)
   Create an instance of this object before calling any of the
   following methods.
   csp_crc32_init(), csp_crc32_update() or csp_crc32_final()
 */
typedef uint32_t csp_crc32_t;

/**
 * Append CRC32 checksum to packet
 *
 * @param[in] packet CSP packet, must be valid.
 * @return #CSP_ERR_NONE on success, otherwise an error code.
 */
int csp_crc32_append(csp_packet_t * packet);

/**
 * Verify CRC32 checksum on packet.
 *
 * @param[in] packet CSP packet, must be valid.
 * @return #CSP_ERR_NONE on success, otherwise an error code.
 */
int csp_crc32_verify(csp_packet_t * packet);

/**
 * Calculate checksum for a given memory area.
 *
 * @param[in] addr memory address
 * @param[in] length length of memory to do checksum on
 * @return checksum
 */
uint32_t csp_crc32_memory(const void * addr, uint32_t length);

/**
   Initialize the CRC32 object prior to calculating checksum.
   @param[in] crc CRC32 object previously created by the caller
*/
void csp_crc32_init(csp_crc32_t *crc);

/**
   Update the CRC32 calculation with a chunk of data
   @param[in] crc CRC32 object previously created, and init'ed by the caller
   @param[in] data pointer to data for which to update checksum on
   @param[in] length number of bytes in the array pointed to by data
*/
void csp_crc32_update(csp_crc32_t * crc, const void * data, uint32_t length);

/**
   Finalize the CRC32 checksum calculation.
   @param[in] crc CRC32 object previously created, init'ed and updated by the caller
   @return checksum
*/
uint32_t csp_crc32_final(csp_crc32_t *crc);

#ifdef __cplusplus
}
#endif
