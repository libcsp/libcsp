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

#include <csp/arch/csp_clock.h>

#include <zephyr.h>
#include <posix/time.h>
#include <logging/log.h>
LOG_MODULE_DECLARE(libcsp);

__attribute__((weak)) void csp_clock_get_time(csp_timestamp_t * time) {
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

__attribute__((weak)) int csp_clock_set_time(const csp_timestamp_t * time) {
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
