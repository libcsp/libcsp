

#pragma once

#include <csp/csp.h>

/**
 *
 * @param packet packet to send - this may not be freed if error code is returned
 * @param from_me 1 if from me, 0 if routed message
 * @return #CSP_ERR_NONE on success, otherwise an error code.
 *
 */

void csp_send_direct(csp_id_t* idout, csp_packet_t * packet, csp_iface_t * routed_from);
void csp_send_direct_iface(const csp_id_t* idout, csp_packet_t * packet, csp_iface_t * iface, uint16_t via, int from_me);
