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

#ifndef _CSP_ROUTE_H_
#define _CSP_ROUTE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include <csp/csp.h>

#include <csp/arch/csp_thread.h>
#include <csp/arch/csp_queue.h>

typedef struct __attribute__((__packed__)) {
	csp_iface_t * interface;
	uint8_t nexthop_mac_addr;
} csp_route_t;

/**
 * csp_route_table_init
 * Initialises the storage for the routing table
 */
int csp_route_table_init(void);

/**
 * Routing table lookup
 * This is the actual lookup in the routing table
 * The table consists of one entry per possible node
 * If there is no explicit nexthop route for the destination
 * the default route (node CSP_DEFAULT_ROUTE) is used.
 */
csp_route_t * csp_route_if(uint8_t id);

/**
 * Interface lookup by name
 * @param name NUL terminated interface name
 * @return pointer to interface or NULL
 */
csp_iface_t * csp_route_get_if_by_name(char *name);

#ifdef CSP_USE_PROMISC
/**
 * Add packet to promiscuous mode packet queue
 *
 * @param packet Packet to add to the queue
 * @param queue Promiscuous mode packet queue
 *
 */
void csp_promisc_add(csp_packet_t * packet, csp_queue_handle_t queue);
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // _CSP_ROUTE_H_
