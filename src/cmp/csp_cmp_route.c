#include "csp_cmp_internal.h"

#include <endian.h>

#include <csp/csp_id.h>
#include <csp/csp_rtable.h>

int csp_cmp_route_set_v1_handler(csp_packet_t * packet) {

	struct csp_cmp_route_set_v1_msg * cmp = (struct csp_cmp_route_set_v1_msg *)packet->data;

	if (csp_cmp_check_len(packet, sizeof(*cmp)) != CSP_ERR_NONE) {
		return CSP_ERR_INVAL;
	}

	csp_iface_t * ifc = csp_iflist_get_by_name(cmp->interface);
	if (ifc == NULL) {
		return CSP_ERR_INVAL;
	}

#if CSP_USE_RTABLE
	if (csp_rtable_set(cmp->dest_node, csp_id_get_host_bits(), ifc, cmp->next_hop_via) != CSP_ERR_NONE) {
		return CSP_ERR_INVAL;
	}
#endif

	packet->length = sizeof(*cmp);

	return CSP_ERR_NONE;
}

int csp_cmp_route_set_v2_handler(csp_packet_t * packet) {

	struct csp_cmp_route_set_v2_msg * cmp = (struct csp_cmp_route_set_v2_msg *)packet->data;

	if (csp_cmp_check_len(packet, sizeof(*cmp)) != CSP_ERR_NONE) {
		return CSP_ERR_INVAL;
	}

	csp_iface_t * ifc = csp_iflist_get_by_name(cmp->interface);
	if (ifc == NULL) {
		return CSP_ERR_INVAL;
	}

#if CSP_USE_RTABLE
	if (csp_rtable_set(be16toh(cmp->dest_node), be16toh(cmp->netmask), ifc, be16toh(cmp->next_hop_via)) != CSP_ERR_NONE) {
		return CSP_ERR_INVAL;
	}
#endif

	packet->length = sizeof(*cmp);

	return CSP_ERR_NONE;
}
