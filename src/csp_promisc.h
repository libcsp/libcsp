

#ifndef _SRC_CSP_PROMISC_H_
#define _SRC_CSP_PROMISC_H_

#include <csp/csp_promisc.h>

/**
 * Add packet to promiscuous mode packet queue
 * @param packet Packet to add to the queue
 */
void csp_promisc_add(csp_packet_t * packet);

#endif
