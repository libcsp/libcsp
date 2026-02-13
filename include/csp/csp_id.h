#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <csp/csp.h>

/**
 * Prepend CSP header fields into the packet's data buffer.
 *
 * This function encodes the CSP ID header into the packet and adjusts the data offset.
 *
 * @param packet Pointer to the packet to modify.
 */
void csp_id_prepend(csp_packet_t * packet);

/**
 * Strip CSP header fields from the packet's data buffer.
 *
 * This function decodes the CSP ID header from the packet and adjusts the data offset.
 *
 * @param packet Pointer to the packet to modify.
 * @return 0 on success, -1 on failure.
 */
int csp_id_strip(csp_packet_t * packet);

/**
 * Setup reception information from CSP ID header.
 *
 * Typically called after stripping the header to configure addressing fields.
 *
 * @param packet Pointer to the received packet.
 * @return 0 on success, -1 on failure.
 */
int csp_id_setup_rx(csp_packet_t * packet);

/**
 * Get number of bits allocated for the host part of the address.
 *
 * @return Number of bits used for the host ID.
 */
unsigned int csp_id_get_host_bits(void);

/**
 * Get maximum allowed node ID.
 *
 * @return Maximum node ID based on current configuration.
 */
unsigned int csp_id_get_max_nodeid(void);

/**
 * Get maximum allowed port number.
 *
 * @return Maximum port number.
 */
unsigned int csp_id_get_max_port(void);

/**
 * Check whether a given address is a broadcast address.
 *
 * @param addr The address to test.
 * @param iface Interface to use for context (netmask, etc.).
 * @return 1 if broadcast, 0 otherwise.
 */
int csp_id_is_broadcast(uint16_t addr, csp_iface_t * iface);

/**
 * Get the size of the CSP header based on the configured version.
 *
 * This function returns the size in bytes of the CSP packet header,
 * depending on whether CSP version 1 or 2 is configured.
 *
 * @return The header size in bytes.
 */
int csp_id_get_header_size(void);

#if (CSP_FIXUP_V1_ZMQ_LITTLE_ENDIAN)

/**
 * Prepend CSPv1-compatible ID header (ZMQ fixup).
 *
 * Used when sending CSPv1 packets on little-endian ZMQ transport.
 *
 * @param packet Pointer to the packet to modify.
 */
void csp_id_prepend_fixup_cspv1(csp_packet_t * packet);

/**
 * Strip CSPv1-compatible ID header (ZMQ fixup).
 *
 * Used when receiving CSPv1 packets on little-endian ZMQ transport.
 *
 * @param packet Pointer to the packet to modify.
 * @return 0 on success, -1 on failure.
 */
int csp_id_strip_fixup_cspv1(csp_packet_t * packet);

#else

/**
 * Wrapper for csp_id_prepend when no fixup is required.
 */
static inline void csp_id_prepend_fixup_cspv1(csp_packet_t * packet) {
	csp_id_prepend(packet);
}

/**
 * Wrapper for csp_id_strip when no fixup is required.
 */
static inline int csp_id_strip_fixup_cspv1(csp_packet_t * packet) {
	return csp_id_strip(packet);
}

#endif

#ifdef __cplusplus
}
#endif
