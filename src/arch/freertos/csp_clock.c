
#include <csp/csp_types.h>
#include <csp/csp_hooks.h>
#include "csp_macro.h"

__weak void csp_clock_get_time(csp_timestamp_t * time) {
	time->tv_sec = 0;
	time->tv_nsec = 0;
}

__weak int csp_clock_set_time(const csp_timestamp_t * time) {
	return CSP_ERR_NOTSUP;
}
