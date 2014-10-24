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

#ifndef RTABLE_H_
#define RTABLE_H_

/**
 * Routing Entry
 */
typedef struct __attribute__((__packed__)) {
	csp_iface_t * interface;
	uint8_t nexthop_mac_addr;
} csp_route_t;

/**
 * Initialises the storage for the routing table
 */
void csp_rtable_init(void);

/**
 * Routing table lookup
 * @param id Host address
 * @return Routing table entry
 */
csp_route_t * csp_route_if(uint8_t id);

/**
 * Setup routing entry
 * @param node Host
 * @param ifc Interface
 * @param nexthop_mac_addr MAC address
 * @return CSP error type
 */
int csp_route_set(uint8_t node, csp_iface_t *ifc, uint8_t nexthop_mac_addr);

void csp_route_print_table(void);

#endif /* RTABLE_H_ */
