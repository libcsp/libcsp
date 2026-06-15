#include "csp_cmp_internal.h"

#include <endian.h>

#include <csp/csp_debug.h>
#include <csp/csp_hooks.h>

int csp_cmp_clock_handler(csp_packet_t * packet) {

	struct csp_cmp_clock_msg * cmp = (struct csp_cmp_clock_msg *)packet->data;

	if (csp_cmp_check_len(packet, sizeof(*cmp)) != CSP_ERR_NONE) {
		return CSP_ERR_INVAL;
	}

	csp_timestamp_t clock;
	clock.tv_sec = be32toh(cmp->clock.tv_sec);
	clock.tv_nsec = be32toh(cmp->clock.tv_nsec);

	int res = CSP_ERR_NONE;
	if (clock.tv_sec != 0) {
		res = csp_clock_set_time(&clock);
		if (res != CSP_ERR_NONE) {
			csp_dbg_errno = CSP_DBG_ERR_CLOCK_SET_FAIL;
		}
	}

	csp_clock_get_time(&clock);

	cmp->clock.tv_sec = htobe32(clock.tv_sec);
	cmp->clock.tv_nsec = htobe32(clock.tv_nsec);

	packet->length = sizeof(*cmp);

	return res;
}
