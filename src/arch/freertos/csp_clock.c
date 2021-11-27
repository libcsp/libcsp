

#include <csp/arch/csp_clock.h>
#include <csp/csp_debug.h>

__attribute__((weak)) void csp_clock_get_time(csp_timestamp_t * time) {
	time->tv_sec = 0;
	time->tv_nsec = 0;
}

__attribute__((weak)) int csp_clock_set_time(const csp_timestamp_t * time) {
	return CSP_ERR_NOTSUP;
}
