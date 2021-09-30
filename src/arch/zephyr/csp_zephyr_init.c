/*
 * Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
 * Copyright (C) 2021 Space Cubics, LLC.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; If not, see <http://www.gnu.org/licenses/>.
 */

#include <zephyr.h>
#include <init.h>
#include <posix/time.h>
#include <csp/csp_debug.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(libcsp);

static void hook_func(csp_debug_level_t level, const char *format, va_list args)
{
	uint32_t args_num = log_count_args(format);

	switch(level) {
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

static int libcsp_zephyr_init(const struct device *unused)
{
	csp_debug_hook_set(hook_func);

	struct timespec ts = {
		.tv_sec = 946652400,
		.tv_nsec = 0,
	};
	clock_settime(CLOCK_REALTIME, &ts);

	return 0;
}

SYS_INIT(libcsp_zephyr_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
