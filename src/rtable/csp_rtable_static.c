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

#include <stdio.h>
#include <csp/csp.h>
#include "rtable.h"
#include "../csp_iflist.h"
#include <stdio.h>

/* Static routing table */
static csp_route_t routes[CSP_ROUTE_COUNT];

void csp_rtable_init(void) {
	memset(routes, 0, sizeof(csp_route_t) * CSP_ROUTE_COUNT);
}

void csp_route_table_load(uint8_t route_table_in[CSP_ROUTE_TABLE_SIZE]) {
	memcpy(routes, route_table_in, sizeof(csp_route_t) * CSP_ROUTE_COUNT);
}

void csp_route_table_save(uint8_t route_table_out[CSP_ROUTE_TABLE_SIZE]) {
	memcpy(route_table_out, routes, sizeof(csp_route_t) * CSP_ROUTE_COUNT);
}

int csp_route_set(uint8_t node, csp_iface_t *ifc, uint8_t nexthop_mac_addr) {

	/* Don't add nothing */
	if (ifc == NULL)
		return CSP_ERR_INVAL;

	/**
	 * Check if the interface has been added.
	 *
	 * NOTE: For future implementations, interfaces should call
	 * csp_route_add_if in its csp_if_<name>_init function, instead
	 * of registering at first route_set, in order to make the interface
	 * available to network based (CMP) route configuration.
	 */
	csp_route_add_if(ifc);

	/* Set route */
	if (node <= CSP_DEFAULT_ROUTE) {
		routes[node].interface = ifc;
		routes[node].nexthop_mac_addr = nexthop_mac_addr;
	} else {
		csp_log_error("Failed to set route: invalid node id %u\r\n", node);
		return CSP_ERR_INVAL;
	}

	return CSP_ERR_NONE;

}

csp_route_t * csp_route_if(uint8_t id) {

	if (routes[id].interface != NULL) {
		return &routes[id];
	} else if (routes[CSP_DEFAULT_ROUTE].interface != NULL) {
		return &routes[CSP_DEFAULT_ROUTE];
	}
	return NULL;

}

#ifdef CSP_DEBUG
void csp_route_print_table(void) {
	int i;
	printf("Node  Interface  Address\r\n");
	for (i = 0; i < CSP_DEFAULT_ROUTE; i++)
		if (routes[i].interface != NULL)
			printf("%4u  %-9s  %u\r\n", i,
				routes[i].interface->name,
				routes[i].nexthop_mac_addr == CSP_NODE_MAC ? i : routes[i].nexthop_mac_addr);
	printf("   *  %-9s  %u\r\n", routes[CSP_DEFAULT_ROUTE].interface->name,
	routes[CSP_DEFAULT_ROUTE].nexthop_mac_addr);

}
#endif
