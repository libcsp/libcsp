

#include <inttypes.h>
#include <string.h>

#include <csp/csp_rtable.h>
#include <csp/csp_debug.h>
#include <csp/csp_id.h>

/* Definition of routing table */
static csp_route_t rtable[CSP_RTABLE_SIZE] = {0};

static int rtable_inptr = 0;

static csp_route_t * csp_rtable_find_exact(uint16_t addr, uint16_t netmask) {

	/* Start search */
	for (int i = 0; i < rtable_inptr; i++) {
		if (rtable[i].address == addr && rtable[i].netmask == netmask) {
			return &rtable[i];
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
	csp_route_t * entry = csp_rtable_find_exact(address, netmask);

	/* If not, create a new one */
	if (!entry) {
		entry = &rtable[rtable_inptr++];
		if (rtable_inptr == CSP_RTABLE_SIZE)
			rtable_inptr = CSP_RTABLE_SIZE;
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

void csp_rtable_iterate(csp_rtable_iterator_t iter, void * ctx) {
	for (int i = 0; i < rtable_inptr; i++) {
		iter(ctx, &rtable[i]);
	}
}
