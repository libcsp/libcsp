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

#include <csp/arch/csp_system.h>
#include <csp/csp_debug.h>

int csp_sys_tasklist(char * out) {
	csp_log_warn("%s() not supported", __func__);

	return 0;
}

int csp_sys_tasklist_size(void) {
	csp_log_warn("%s() not supported", __func__);

	return 0;
}

uint32_t csp_sys_memfree(void) {
	csp_log_warn("%s() not supported", __func__);

	return 0;
}

void csp_sys_set_color(csp_color_t color) {
	/* not implemented. won't be used. */
}
