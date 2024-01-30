

#include <inttypes.h>
#include <string.h>

#include <csp/csp_rtable.h>
#include <csp/csp_debug.h>
#include <csp/csp_id.h>

/* Definition of routing table */
static csp_route_t rtable[CSP_RTABLE_SIZE] = {0};

static int rtable_inptr = 0;

static csp_route_t * csp_rtable_find_exact(uint16_t addr, uint16_t netmask, csp_iface_t * ifc) {

	/* Start search */
	for (int i = 0; i < rtable_inptr; i++) {
		if (rtable[i].address == addr && rtable[i].netmask == netmask && rtable[i].iface == ifc) {
			return &rtable[i];
		}
	}

	return NULL;
}

csp_route_t * csp_rtable_search_backward(csp_route_t * start_route) {

    if (start_route == NULL || start_route <= rtable) {
        return NULL;
    }

    /* Start searching backward from the route before start_route */
    for (csp_route_t * route = start_route - 1; route >= rtable; route--) {

        if (route->netmask == start_route->netmask && route->address == start_route->address) {
            return route;
        }
    }

    return NULL;
}

csp_route_t * csp_rtable_find_route(uint16_t addr) {

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
		return &rtable[best_result];
	}

	return NULL;
}

int csp_rtable_set_internal(uint16_t address, uint16_t netmask, csp_iface_t * ifc, uint16_t via) {

	/* First see if the entry exists */
	csp_route_t * entry = csp_rtable_find_exact(address, netmask, ifc);

	/* If not, create a new one */
	if (!entry) {
		entry = &rtable[rtable_inptr++];
		if (rtable_inptr > CSP_RTABLE_SIZE) {
			rtable_inptr = CSP_RTABLE_SIZE;
		}
	}

	/* Fill in the data */
	entry->address = address;
	entry->netmask = netmask;
	entry->iface = ifc;
	entry->via = via;

	return CSP_ERR_NONE;
}

void csp_rtable_free(void) {
	memset(rtable, 0, sizeof(rtable));
}

void csp_rtable_clear(void) {
	csp_rtable_free();
}

int csp_rtable_set(uint16_t address, int netmask, csp_iface_t * ifc, uint16_t via) {

	if ((netmask < 0) || (netmask > (int)csp_id_get_host_bits())) {
		netmask = csp_id_get_host_bits();
	}

	/* Validates options */
	if (ifc == NULL) {
		csp_dbg_errno = CSP_DBG_ERR_INVALID_RTABLE_ENTRY;
		return CSP_ERR_INVAL;
	}

	return csp_rtable_set_internal(address, netmask, ifc, via);
}

void csp_rtable_iterate(csp_rtable_iterator_t iter, void * ctx) {
	for (int i = 0; i < rtable_inptr; i++) {
		iter(ctx, &rtable[i]);
	}
}

#if (CSP_ENABLE_CSP_PRINT)

static bool csp_rtable_print_route(void * ctx, csp_route_t * route) {
	if (route->via == CSP_NO_VIA_ADDRESS) {
		csp_print("%u/%u %s\r\n", route->address, route->netmask, route->iface->name);
	} else {
		csp_print("%u/%u %s %u\r\n", route->address, route->netmask, route->iface->name, route->via);
	}
	return true;
}

void csp_rtable_print(void) {
	csp_rtable_iterate(csp_rtable_print_route, NULL);
}

#endif
