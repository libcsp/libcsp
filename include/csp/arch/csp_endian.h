/****************************************************************************
 * **File:** csp/arch/csp_endian.h
 *
 * **Description:** CSP endian handling
 ****************************************************************************/
#pragma once

#include "csp/autoconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

#if HAVE_ENDIAN_H
#include <endian.h>
#else
#if CSP_ZEPHYR
#include <zephyr/sys/byteorder.h>
/* Map Zephyr's byteorder functions to standard endian.h names */
#define htobe16(x) sys_cpu_to_be16(x)
#define htole16(x) sys_cpu_to_le16(x)
#define be16toh(x) sys_be16_to_cpu(x)
#define le16toh(x) sys_le16_to_cpu(x)

#define htobe32(x) sys_cpu_to_be32(x)
#define htole32(x) sys_cpu_to_le32(x)
#define be32toh(x) sys_be32_to_cpu(x)
#define le32toh(x) sys_le32_to_cpu(x)

#define htobe64(x) sys_cpu_to_be64(x)
#define htole64(x) sys_cpu_to_le64(x)
#define be64toh(x) sys_be64_to_cpu(x)
#define le64toh(x) sys_le64_to_cpu(x)
#elif CSP_WINDOWS || CSP_FREERTOS || CSP_POSIX
#include <stdint.h>
// Declare functions for 32-bit and 64-bit conversions
uint16_t htobe16(uint16_t host_16bits);
uint16_t be16toh(uint16_t big_endian_16bits);
uint16_t htole16(uint16_t host_16bits);
uint16_t le16toh(uint16_t little_endian_16bits);

uint32_t htobe32(uint32_t host_32bits);
uint32_t be32toh(uint32_t big_endian_32bits);
uint32_t htole32(uint32_t host_32bits);
uint32_t le32toh(uint32_t little_endian_32bits);

uint64_t htobe64(uint64_t host_64bits);
uint64_t be64toh(uint64_t big_endian_64bits);
uint64_t htole64(uint64_t host_64bits);
uint64_t le64toh(uint64_t little_endian_64bits);

// Declare functions for 16-bit conversions
#else
#error Platform not supported for endian conversion
#endif
#endif

#ifdef __cplusplus
}
#endif
