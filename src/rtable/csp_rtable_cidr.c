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
#include <csp/arch/csp_malloc.h>
#include <csp/csp_id.h>

/* Definition of routing table */
typedef struct csp_rtable_s {
    csp_route_t route;
    uint16_t address;
    uint16_t netmask;
    struct csp_rtable_s * next;
} csp_rtable_t;

/* Routing table (linked list) */
static csp_rtable_t * rtable = NULL;

static csp_rtable_t * csp_rtable_find_exact(uint16_t addr, uint16_t netmask) {

	/* Start search */
	csp_rtable_t * i = rtable;
	while(i) {
		if (i->address == addr && i->netmask == netmask) {
			return i;
		}
		i = i->next;
	}

	return NULL;

}

const csp_route_t * csp_rtable_find_route(uint16_t addr) {

	/* Remember best result */
	csp_rtable_t * best_result = NULL;
	uint16_t best_result_mask = 0;

	/* Start search */
	csp_rtable_t * i = rtable;
	while(i) {

		uint16_t hostbits = (1 << (csp_id_get_host_bits() - i->netmask)) - 1;
		uint16_t netbits = ~hostbits;

		/* Match network addresses */
		uint16_t net_a = i->address & netbits;
		uint16_t net_b = addr & netbits;
		//printf("route %u/%u %s, netbits %x, hostbits %x, (A & netbits): %hu, (B & netbits): %hu\r\n", i->address, i->netmask, i->route.iface->name, netbits, hostbits, net_a, net_b);

		/* We have a match */
		if (net_a == net_b) {
			if (i->netmask >= best_result_mask) {
				best_result = i;
				best_result_mask = i->netmask;
			}
		}

		i = i->next;

	}

	if (best_result) {
		//csp_log_info("Using routing entry: %u/%u if %s via %u",best_result->address, best_result->netmask, best_result->route.iface->name, best_result->route.via);
		return &best_result->route;
	}

	return NULL;

}

int csp_rtable_set_internal(uint16_t address, uint16_t netmask, csp_iface_t *ifc, uint16_t via) {

	/* First see if the entry exists */
	csp_rtable_t * entry = csp_rtable_find_exact(address, netmask);

	/* If not, create a new one */
	if (!entry) {
		entry = csp_malloc(sizeof(*entry));
		if (entry == NULL) {
			return CSP_ERR_NOMEM;
		}

		entry->next = NULL;
		/* Add entry to linked-list */
		if (rtable == NULL) {
			/* This is the first interface to be added */
			rtable = entry;
		} else {
			/* One or more interfaces were already added */
			csp_rtable_t * i = rtable;
			while (i->next) {
				i = i->next;
			}
			i->next = entry;
		}
	}

	/* Fill in the data */
	entry->address = address;
	entry->netmask = netmask;
	entry->route.iface = ifc;
	entry->route.via = via;

	return CSP_ERR_NONE;
}

void csp_rtable_free(void) {
	for (csp_rtable_t * i = rtable; (i);) {
		void * freeme = i;
		i = i->next;
		csp_free(freeme);
	}
	rtable = NULL;
}

void csp_rtable_iterate(csp_rtable_iterator_t iter, void * ctx)
{
    for (csp_rtable_t * route = rtable;
         route && iter(ctx, route->address, route->netmask, &route->route);
         route = route->next);
}
