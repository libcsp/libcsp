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

/** @brief Connection struct */
struct csp_conn_s {
    csp_conn_state_t state;         // Connection state (SOCKET_OPEN or SOCKET_CLOSED)
    csp_id_t idin;                  // Identifier received
    csp_id_t idout;                 // Identifier transmitted
    csp_queue_handle_t rx_queue;    // Queue for RX packets
    csp_queue_handle_t rx_socket;	// Socket to be "woken" when first packet is ready
    csp_l4data_t * l4data;			// Pointer to a layer4 info area (Opaque pointer)
    uint32_t open_timestamp;		// Time the connection was opened
    uint32_t conn_opts;				// Connection options
};

/** @brief Socket struct */
struct csp_socket_s {
    csp_queue_handle_t conn_queue;	// Connection queue handle
    uint32_t opts;					// Socket options
};

void csp_conn_init(void);
csp_conn_t * csp_conn_find(uint32_t id, uint32_t mask);
csp_conn_t * csp_conn_new(csp_id_t idin, csp_id_t idout);
void csp_close_wait(csp_conn_t * conn);
void csp_conn_check_timeouts(void);

#endif // _CSP_CONN_H_
