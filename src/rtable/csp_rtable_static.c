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
#include <stdio.h>

/* Local typedef for routing table */
typedef struct __attribute__((__packed__)) csp_rtable_s {
	csp_iface_t * interface;
	uint8_t mac;
} csp_rtable_t;

/* Static storage context for routing table */
static csp_rtable_t routes[CSP_ROUTE_COUNT] = {};

/**
 * Find entry in static routing table
 * This is done by table lookup with fallback to the default route
 * The reason why the csp_rtable_t struct is not returned directly
 * is that we wish to hide the storage format, mainly because of
 * the alternative routing table storage (cidr).
 * @param id Node
 * @return pointer to found routing entry
 */
static csp_rtable_t * csp_rtable_find(uint8_t id) {

	if (routes[id].interface != NULL) {
		return &routes[id];
	} else if (routes[CSP_DEFAULT_ROUTE].interface != NULL) {
		return &routes[CSP_DEFAULT_ROUTE];
	}
	return NULL;

}

csp_iface_t * csp_rtable_find_iface(uint8_t id) {
	csp_rtable_t * route = csp_rtable_find(id);
	if (route == NULL)
		return NULL;
	return route->interface;
}

uint8_t csp_rtable_find_mac(uint8_t id) {
	csp_rtable_t * route = csp_rtable_find(id);
	if (route == NULL)
		return 255;
	return route->mac;
}

void csp_rtable_clear(void) {
	memset(routes, 0, sizeof(routes[0]) * CSP_ROUTE_COUNT);
}

void csp_route_table_load(uint8_t route_table_in[CSP_ROUTE_TABLE_SIZE]) {
	memcpy(routes, route_table_in, sizeof(routes[0]) * CSP_ROUTE_COUNT);
}

void csp_route_table_save(uint8_t route_table_out[CSP_ROUTE_TABLE_SIZE]) {
	memcpy(route_table_out, routes, sizeof(routes[0]) * CSP_ROUTE_COUNT);
}

int csp_rtable_set(uint8_t node, uint8_t mask, csp_iface_t *ifc, uint8_t mac) {

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
	csp_iflist_add(ifc);

	/* Set route */
	if (node <= CSP_DEFAULT_ROUTE) {
		routes[node].interface = ifc;
		routes[node].mac = mac;
	} else {
		csp_log_error("Failed to set route: invalid node id %u", node);
		return CSP_ERR_INVAL;
	}

	return CSP_ERR_NONE;

}

#ifdef CSP_DEBUG
void csp_rtable_print(void) {
	int i;
	printf("Node  Interface  Address\r\n");
	for (i = 0; i < CSP_DEFAULT_ROUTE; i++)
		if (routes[i].interface != NULL)
			printf("%4u  %-9s  %u\r\n", i,
				routes[i].interface->name,
				routes[i].mac == CSP_NODE_MAC ? i : routes[i].mac);
	printf("   *  %-9s  %u\r\n", routes[CSP_DEFAULT_ROUTE].interface->name, routes[CSP_DEFAULT_ROUTE].mac);

}
#endif
