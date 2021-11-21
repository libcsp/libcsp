

#pragma once

#include <csp/csp.h>

/**
   Send CSP packet via route (no existing connection).

   @param idout 32bit CSP identifier
   @param packet packet to send - this will not be freed.
   @param ifroute route to destination
   @param from_me 1 if from me, 0 if routed message
   @return #CSP_ERR_NONE on success, otherwise an error code.
*/
int csp_send_direct(csp_id_t idout, csp_packet_t * packet, const csp_route_t * ifroute, int from_me);
