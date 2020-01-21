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

#include "csp_rtable_internal.h"

#include <csp/csp_debug.h>

/* Routing table (static array) */
static csp_route_t rtable[CSP_DEFAULT_ROUTE + 1] = {};

const csp_route_t * csp_rtable_find_route(uint8_t dest_address) {

	if (rtable[dest_address].iface != NULL) {
		return &rtable[dest_address];
	}
	if (rtable[CSP_DEFAULT_ROUTE].iface != NULL) {
		return &rtable[CSP_DEFAULT_ROUTE];
	}
	return NULL;

}

int csp_rtable_set_internal(uint8_t address, uint8_t netmask, csp_iface_t *ifc, uint8_t via) {

	/* Validates options */
	if ((netmask != 0) && (netmask != CSP_ID_HOST_SIZE)) {
		csp_log_error("%s: invalid netmask in route: address %u, netmask %u, interface %p, via %u", __FUNCTION__, address, netmask, ifc, via);
		return CSP_ERR_INVAL;
	}

	/* Set route */
        const unsigned int ri = (netmask == 0) ? CSP_DEFAULT_ROUTE : address;
        rtable[ri].iface = ifc;
        rtable[ri].via = via;

	return CSP_ERR_NONE;
}

void csp_rtable_free(void) {

	memset(rtable, 0, sizeof(rtable));
}

void csp_rtable_iterate(csp_rtable_iterator_t iter, void * ctx) {

	for (unsigned int i = 0; i < CSP_DEFAULT_ROUTE; ++i) {
		if (rtable[i].iface != NULL) {
			if (iter(ctx, i, CSP_ID_HOST_SIZE, &rtable[i]) == false) {
				return; // stopped by user
			}
		}
	}
	if (rtable[CSP_DEFAULT_ROUTE].iface) {
		iter(ctx, 0, 0, &rtable[CSP_DEFAULT_ROUTE]);
	}
}
