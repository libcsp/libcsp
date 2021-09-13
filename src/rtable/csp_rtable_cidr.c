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

#include <inttypes.h>
#include <malloc.h>
#include <string.h>

#include <csp/csp_rtable.h>
#include <csp/csp_id.h>

/* Definition of routing table */
static struct csp_rtable_s {
    csp_route_t route;
    uint16_t address;
    uint16_t netmask;
} rtable[CSP_RTABLE_SIZE] = {0};

static int rtable_inptr = 0;

static struct csp_rtable_s * csp_rtable_find_exact(uint16_t addr, uint16_t netmask) {

	/* Start search */
	for (int i = 0; i < rtable_inptr; i++) {
		if (rtable[i].address == addr && rtable[i].netmask == netmask) {
			return &rtable[i];
		}
	}

	return NULL;

}

const csp_route_t * csp_rtable_find_route(uint16_t addr) {

	/* Remember best result */
	int best_result = -1;
	uint16_t best_result_mask = 0;

	/* Start search */
	for (int i = 0; i < rtable_inptr; i++) {

		uint16_t hostbits = (1 << (csp_id_get_host_bits() - rtable[i].netmask)) - 1;
		uint16_t netbits = ~hostbits;

		/* Match network addresses */
		uint16_t net_a = rtable[i].address & netbits;
		uint16_t net_b = addr & netbits;

		/* We have a match */
		if (net_a == net_b) {
			if (rtable[i].netmask >= best_result_mask) {
				best_result = i;
				best_result_mask = rtable[i].netmask;
			}
		}

	}

	if (best_result > -1) {
		return &rtable[best_result].route;
	}

	return NULL;

}

int csp_rtable_set_internal(uint16_t address, uint16_t netmask, csp_iface_t *ifc, uint16_t via) {

	/* First see if the entry exists */
	struct csp_rtable_s * entry = csp_rtable_find_exact(address, netmask);

	/* If not, create a new one */
	if (!entry) {
		entry = &rtable[rtable_inptr++];
		if (rtable_inptr == CSP_RTABLE_SIZE)
			rtable_inptr = CSP_RTABLE_SIZE;
	}

	/* Fill in the data */
	entry->address = address;
	entry->netmask = netmask;
	entry->route.iface = ifc;
	entry->route.via = via;

	return CSP_ERR_NONE;
}

void csp_rtable_free(void) {
	memset(rtable, 0, sizeof(rtable));
}

void csp_rtable_iterate(csp_rtable_iterator_t iter, void * ctx) {
	for (int i = 0; i < rtable_inptr; i++) {
		iter(ctx, rtable[i].address, rtable[i].netmask, &rtable[i].route);
	}
}
