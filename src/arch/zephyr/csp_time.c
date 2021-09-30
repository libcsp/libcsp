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

#include <csp/arch/csp_time.h>
#include <zephyr.h>

uint32_t csp_get_ms(void)
{
	return k_uptime_get_32();
}

uint32_t csp_get_ms_isr(void)
{
	return k_uptime_get_32();
}

uint32_t csp_get_s(void)
{
	return k_uptime_get_32() / MSEC_PER_SEC;
}

uint32_t csp_get_s_isr(void)
{
	return k_uptime_get_32() / MSEC_PER_SEC;
}
