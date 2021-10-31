

#pragma once

/**
   @file
   CRC32 support.
*/

#include <csp/csp.h>

/**
   Append CRC32 checksum to packet
   @param[in] packet CSP packet, must be valid.
   @return #CSP_ERR_NONE on success, otherwise an error code.
*/
int csp_crc32_append(csp_packet_t * packet);

/**
   Verify CRC32 checksum on packet.
   @param[in] packet CSP packet, must be valid.
   @return #CSP_ERR_NONE on success, otherwise an error code.
*/
int csp_crc32_verify(csp_packet_t * packet);

/**
   Calculate checksum for a given memory area.
   @param[in] addr memory address
   @param[in] length length of memory to do checksum on
   @return checksum
*/
uint32_t csp_crc32_memory(const uint8_t * addr, uint32_t length);

