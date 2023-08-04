#include <csp/csp_types.h>
#include <zephyr/kernel.h>
#include <zephyr/posix/time.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(libcsp);

__weak void csp_clock_get_time(csp_timestamp_t * time) {
	struct timespec ts;
	int ret;

	ret = clock_gettime(CLOCK_REALTIME, &ts);
	if (ret < 0) {
		LOG_WRN("clock_gettime() failed, retruning with 0s");
		time->tv_sec = 0;
		time->tv_nsec = 0;
	} else {
		time->tv_sec = ts.tv_sec;
		time->tv_nsec = ts.tv_nsec;
	}
}

__weak int csp_clock_set_time(const csp_timestamp_t * time) {
	int ret;
	struct timespec ts;

	ts.tv_sec = time->tv_sec;
	ts.tv_nsec = time->tv_nsec;

	ret = clock_settime(CLOCK_REALTIME, &ts);
	if (ret < 0) {
		return CSP_ERR_INVAL;
	}

	return CSP_ERR_NONE;
}
