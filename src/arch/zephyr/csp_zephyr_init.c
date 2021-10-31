

#include <zephyr.h>
#include <init.h>
#include <posix/time.h>
#include <csp/csp_debug.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(libcsp);

static void hook_func(csp_debug_level_t level, const char * format, va_list args) {
	uint32_t args_num = log_count_args(format);

	switch (level) {
		case CSP_ERROR:
			Z_LOG_VA(LOG_LEVEL_ERR, format, args, args_num, LOG_STRDUP_EXEC);
			break;
		case CSP_WARN:
			Z_LOG_VA(LOG_LEVEL_WRN, format, args, args_num, LOG_STRDUP_EXEC);
			break;
		default:
			Z_LOG_VA(LOG_LEVEL_INF, format, args, args_num, LOG_STRDUP_EXEC);
			break;
	}
}

static int libcsp_zephyr_init(const struct device * unused) {
	csp_debug_hook_set(hook_func);

	struct timespec ts = {
		.tv_sec = 946652400,
		.tv_nsec = 0,
	};
	clock_settime(CLOCK_REALTIME, &ts);

	return 0;
}

SYS_INIT(libcsp_zephyr_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
