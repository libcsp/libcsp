#include "csp/arch/csp_endian.h"
#include <stdint.h>

// Helper function to swap bytes in 16-bit unsigned integer
static inline uint16_t swap_uint16(uint16_t val) {
	return (((val & 0xff00) >> 8) |
			((val & 0x00ff) << 8));
}

// Helper function to swap bytes in 32-bit unsigned integer
static inline uint32_t swap_uint32(uint32_t val) {
	return (((val & 0xff000000) >> 24) |
			((val & 0x000000ff) << 24) |
			((val & 0x0000ff00) << 8) |
			((val & 0x00ff0000) >> 8));
}

// Helper function to swap bytes in 64-bit unsigned integer
static inline uint64_t swap_uint64(uint64_t val) {
	return (((val & 0xff00000000000000LL) >> 56) |
			((val & 0x00000000000000ffLL) << 56) |
			((val & 0x00ff000000000000LL) >> 40) |
			((val & 0x000000000000ff00LL) << 40) |
			((val & 0x0000ff0000000000LL) >> 24) |
			((val & 0x0000000000ff0000LL) << 24) |
			((val & 0x000000ff00000000LL) >> 8) |
			((val & 0x00000000ff000000LL) << 8));
}

// Conversion functions for 16-bit, 32-bit, and 64-bit integers
uint16_t htobe16(uint16_t host_16bits) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return swap_uint16(host_16bits);
#else
	return host_16bits;
#endif
}

uint32_t htobe32(uint32_t host_32bits) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return swap_uint32(host_32bits);
#else
	return host_32bits;
#endif
}

uint64_t htobe64(uint64_t host_64bits) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return swap_uint64(host_64bits);
#else
	return host_64bits;
#endif
}

uint16_t be16toh(uint16_t big_endian_16bits) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return swap_uint16(big_endian_16bits);
#else
	return big_endian_16bits;
#endif
}

uint32_t be32toh(uint32_t big_endian_32bits) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return swap_uint32(big_endian_32bits);
#else
	return big_endian_32bits;
#endif
}

uint64_t be64toh(uint64_t big_endian_64bits) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return swap_uint64(big_endian_64bits);
#else
	return big_endian_64bits;
#endif
}

uint32_t htole32(uint32_t host_32bits) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return host_32bits;
#else
	return swap_uint32(host_32bits);
#endif
}

uint32_t le32toh(uint32_t little_endian_32bits) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return little_endian_32bits;
#else
	return swap_uint32(little_endian_32bits);
#endif
}

uint16_t htole16(uint16_t host_16bits) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return host_16bits;
#else
	return swap_uint16(host_16bits);
#endif
}

uint16_t le16toh(uint16_t little_endian_16bits) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return little_endian_16bits;
#else
	return swap_uint16(little_endian_16bits);
#endif
}

uint64_t htole64(uint64_t host_64bits) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return host_64bits;
#else
	return swap_uint64(host_64bits);
#endif
}

uint64_t le64toh(uint64_t little_endian_64bits) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return little_endian_64bits;
#else
	return swap_uint64(little_endian_64bits);
#endif
}
