#include "csp_cmp_internal.h"

int csp_cmp_handler(csp_packet_t * packet) {

	if (csp_cmp_check_len(packet, sizeof(struct csp_cmp_header)) != CSP_ERR_NONE) {
		return CSP_ERR_INVAL;
	}

	struct csp_cmp_header * cmp = (struct csp_cmp_header *)packet->data;

	if (cmp->type != CSP_CMP_REQUEST) {
		return CSP_ERR_INVAL;
	}

	int ret;
	switch (cmp->code) {
		case CSP_CMP_IDENT:
			ret = csp_cmp_ident_handler(packet);
			break;

		case CSP_CMP_ROUTE_SET_V1:
			ret = csp_cmp_route_set_v1_handler(packet);
			break;

		case CSP_CMP_ROUTE_SET_V2:
			ret = csp_cmp_route_set_v2_handler(packet);
			break;

		case CSP_CMP_IF_STATS:
			ret = csp_cmp_if_stats_handler(packet);
			break;

		case CSP_CMP_PEEK:
			ret = csp_cmp_peek_handler(packet);
			break;

		case CSP_CMP_POKE:
			ret = csp_cmp_poke_handler(packet);
			break;

		case CSP_CMP_PEEK_V2:
			ret = csp_cmp_peek_v2_handler(packet);
			break;

		case CSP_CMP_POKE_V2:
			ret = csp_cmp_poke_v2_handler(packet);
			break;

		case CSP_CMP_CLOCK:
			ret = csp_cmp_clock_handler(packet);
			break;

		case CSP_CMP_TIME_SYNC:
			ret = csp_cmp_time_sync_handler(packet);
			break;

		case CSP_CMP_TIME_FUP:
			ret = csp_cmp_time_fup_handler(packet);
			break;

		default:
			return CSP_ERR_INVAL;
	}

	if (ret == CSP_ERR_NONE) {
		cmp->type = CSP_CMP_REPLY;
	}

	return ret;
}
