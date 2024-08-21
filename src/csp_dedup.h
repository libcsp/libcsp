#pragma once

#include <csp/csp_types.h>

/**
 * Check for a duplicate packet
 * @param packet pointer to packet
 * @return false if not a duplicate, true if duplicate
 */
bool csp_dedup_is_duplicate(csp_packet_t * packet);
