#pragma once

#include <csp/csp.h>

/**
 * Send a packet, resolving the outgoing interface(s) from the destination
 * address in @p idout.
 *
 * The packet is always consumed by this call, either handed over to one or
 * more interfaces or freed on error.
 *
 * @param idout header (identifier) to send with
 * @param packet packet to send
 * @param routed_from interface the packet was received on, or NULL if the
 *        packet originates from the local node
 */
void csp_send_direct(csp_id_t* idout, csp_packet_t * packet, csp_iface_t * routed_from);

/**
 * Send a packet on a specific interface.
 *
 * The packet is always consumed by this call, either handed over to the
 * interface or freed on error.
 *
 * @param idout header (identifier) to send with
 * @param packet packet to send
 * @param iface interface to send the packet on
 * @param via via address, or #CSP_NO_VIA_ADDRESS to send directly to the destination
 * @param from_me 1 if the packet originates from the local node, 0 if it is a routed message
 */
void csp_send_direct_iface(const csp_id_t* idout, csp_packet_t * packet, csp_iface_t * iface, uint16_t via, int from_me);
