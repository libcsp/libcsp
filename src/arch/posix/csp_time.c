

#include <csp/arch/csp_time.h>

#include <time.h>
#include <sys/time.h>
#include <limits.h>

uint32_t csp_get_ms(void) {

	struct timespec ts;
	if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
		return (uint32_t)((ts.tv_sec * 1000) + (ts.tv_nsec / 1000000));
	}
	return 0;
}

uint32_t csp_get_ms_isr(void) {

	return csp_get_ms();
}

uint32_t csp_get_s(void) {

	struct timespec ts;
	if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
		return (uint32_t)ts.tv_sec;
	}
	return 0;
}

uint32_t csp_get_s_isr(void) {

	return csp_get_s();
}
