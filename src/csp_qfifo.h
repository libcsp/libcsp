/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2012 GomSpace ApS (http://www.gomspace.com)
Copyright (C) 2012 AAUSAT3 Project (http://aausat3.space.aau.dk)

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

#ifndef CSP_QFIFO_H_
#define CSP_QFIFO_H_

#include <csp/csp_interface.h>

#if (CSP_USE_RDP)
#define FIFO_TIMEOUT 100			//! If RDP is enabled, the router needs to awake some times to check timeouts
#else
#define FIFO_TIMEOUT CSP_MAX_TIMEOUT		//! If no RDP, the router can sleep untill data arrives
#endif

/**
 * Init FIFO/QOS queues
 * @return CSP_ERR type
 */
int csp_qfifo_init(void);

void csp_qfifo_free_resources(void);

typedef struct {
	csp_iface_t * iface;
	csp_packet_t * packet;
} csp_qfifo_t;

/**
 * Read next packet from router input queue
 * @param input pointer to router queue item element
 * @return CSP_ERR type
 */
int csp_qfifo_read(csp_qfifo_t * input);

/**
 * Wake up any task (e.g. router) waiting on messages.
 * For testing.
 */
void csp_qfifo_wake_up(void);

#endif /* CSP_QFIFO_H_ */
