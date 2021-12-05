
#include <csp/csp_types.h>
#include <csp/csp_hooks.h>

__attribute__((weak)) void csp_clock_get_time(csp_timestamp_t * time) {
	time->tv_sec = 0;
	time->tv_nsec = 0;
}

__attribute__((weak)) int csp_clock_set_time(const csp_timestamp_t * time) {
	return CSP_ERR_NOTSUP;
}
