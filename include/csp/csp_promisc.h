/****************************************************************************
 * **File:** csp/csp_promisc.h
 *
 * **Description:** Promiscuous packet queue. This feature is used to enable
 * promiscuous mode for incoming packets, e.g. router, bridge. If enabled, a copy
 * of all incoming packets are cloned (using csp_buffer_clone()) and placed in a
 * FIFO queue, that can be read using csp_promisc_read().
 ****************************************************************************/
#pragma once

#include <csp/csp_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Enable promiscuous packet queue.
 *
 * @param[in]queue_size: Size (max length) of queue for incoming packets.
 * @return #CSP_ERR_NONE on success, otherwise an error code.
 */
int csp_promisc_enable(unsigned int queue_size);

/**
 * Disable promiscuous mode.
 */
void csp_promisc_disable(void);

/**
 * Get/dequeue packet from promiscuous packet queue.
 *
 * Returns the first packet from the promiscuous packet queue.
 *
 * @param[in] timeout Timeout in ms to wait for a packet.
 * @return Packet (free with csp_buffer_free() or re-use packet), NULL on error or timeout.
 */
csp_packet_t *csp_promisc_read(uint32_t timeout);

#ifdef __cplusplus
}
#endif
