

#include <zephyr.h>
#include <init.h>
#include <posix/time.h>
#include <csp/csp_debug.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(libcsp);

static int libcsp_zephyr_init(const struct device * unused) {

	struct timespec ts = {
		.tv_sec = 946652400,
		.tv_nsec = 0,
	};
	clock_settime(CLOCK_REALTIME, &ts);

	return 0;
}

SYS_INIT(libcsp_zephyr_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
