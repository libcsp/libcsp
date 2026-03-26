#include <csp/csp_hooks.h>
#include "csp_macro.h"

#include <time.h>

__weak void csp_clock_get_time(csp_timestamp_t * time) {

	struct timespec ts;
	if (clock_gettime(CLOCK_REALTIME, &ts) == 0) {
		time->tv_sec = ts.tv_sec;
		time->tv_nsec = ts.tv_nsec;
	} else {
		time->tv_sec = 0;
		time->tv_nsec = 0;
	}
}

__weak int csp_clock_set_time(const csp_timestamp_t * time) {

	struct timespec ts = {.tv_sec = time->tv_sec, .tv_nsec = time->tv_nsec};
	if (clock_settime(CLOCK_REALTIME, &ts) == 0) {
		return CSP_ERR_NONE;
	}
	return CSP_ERR_INVAL;  // CSP doesn't have any matching error codes
}
