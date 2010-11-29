/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2010 GomSpace ApS (gomspace.com)
Copyright (C) 2010 AAUSAT3 Project (aausat3.space.aau.dk) 

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef _CSP_CONN_H_
#define _CSP_CONN_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include <csp/csp.h>

#include "arch/csp_queue.h"
#include "arch/csp_semaphore.h"

/** @brief Connection states */
typedef enum {
    CONN_CLOSED = 0,				// Connection closed
    CONN_OPEN = 1,					// Connection open
    CONN_CLOSE_WAIT = 2,			// Conneciton closed by network stack, waiting for userspace to close too.
} csp_conn_state_t;

typedef enum {
	RDP_CLOSED = 0,
	RDP_LISTEN,
	RDP_SYN_SENT,
	RDP_SYN_RCVD,
	RDP_OPEN,
	RDP_CLOSE_WAIT,
} csp_rdp_state_t;

/** @brief RDP Connection header
 *  @note Do not try to pack this stuct, the posix sem handle will stop working */
typedef struct {
	csp_rdp_state_t state;				/**< Connection state */
	uint16_t snd_nxt; 					/**< The sequence number of the next segment that is to be sent */
	uint16_t snd_una; 					/**< The sequence number of the oldest unacknowledged segment */
	uint16_t snd_iss; 					/**< The initial send sequence number */
	uint16_t rcv_cur; 					/**< The sequence number of the last segment received correctly and in sequence */
	uint16_t rcv_irs; 					/**< The initial receive sequence number */
	uint16_t rcv_lsa; 					/**< The last sequence number acknowledged by the receiver */
	uint32_t window_size;
	uint32_t conn_timeout;
	uint32_t packet_timeout;
	uint32_t delayed_acks;
	uint32_t ack_timeout;
	uint32_t ack_delay_count;
	uint32_t ack_timestamp;
	csp_bin_sem_handle_t tx_wait;
	csp_queue_handle_t tx_queue;
	csp_queue_handle_t rx_queue;
} csp_rdp_t;

/** @brief Connection struct */
struct csp_conn_s {
    csp_conn_state_t state;         // Connection state (SOCKET_OPEN or SOCKET_CLOSED)
    csp_id_t idin;                  // Identifier received
    csp_id_t idout;                 // Identifier transmitted
    csp_queue_handle_t rx_queue;    // Queue for RX packets
    csp_queue_handle_t rx_socket;	// Socket to be "woken" when first packet is ready
    uint32_t open_timestamp;		// Time the connection was opened
    uint32_t conn_opts;				// Connection options
#if CSP_USE_RDP
    csp_rdp_t rdp;					// RDP state
#endif
};

/** @brief Socket struct */
struct csp_socket_s {
    csp_queue_handle_t conn_queue;	// Connection queue handle
    uint32_t opts;					// Socket options
};

void csp_conn_init(void);
csp_conn_t * csp_conn_find(uint32_t id, uint32_t mask);
csp_conn_t * csp_conn_new(csp_id_t idin, csp_id_t idout);
void csp_conn_check_timeouts(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // _CSP_CONN_H_
