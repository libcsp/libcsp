/****************************************************************************
 * File: csp_crc32.h
 * Description: CRC32 support.
 ****************************************************************************/
#pragma once

#include <csp/csp.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Append CRC32 checksum to packet
 *
 * Parameters:
 *	packet (csp_packet_t *) [in]: CSP packet, must be valid.
 *
 * Returns:
 *	int: #CSP_ERR_NONE on success, otherwise an error code.
 */
int csp_crc32_append(csp_packet_t * packet);

/**
 * Verify CRC32 checksum on packet.
 *
 * Parameters:
 *	packet (csp_packet_t *) [in]: CSP packet, must be valid.
 *
 * Returns:
 *	int: #CSP_ERR_NONE on success, otherwise an error code.
 */
int csp_crc32_verify(csp_packet_t * packet);

/**
 * Calculate checksum for a given memory area.
 *
 * Parameters:
 *	addr (const uint8_t *) [in]: memory address
 *	length (uint32_t) [in]: length of memory to do checksum on
 *
 * Returns:
 *	uint32_t: checksum
 */
uint32_t csp_crc32_memory(const uint8_t * addr, uint32_t length);

#ifdef __cplusplus
}
#endif
