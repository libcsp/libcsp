/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2012 Gomspace ApS (http://www.gomspace.com)
Copyright (C) 2012 AAUSAT3 Project (http://aausat3.space.aau.dk)

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <csp/csp_debug.h>

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#ifdef __AVR__
#include <avr/pgmspace.h>
#endif

#include <csp/csp.h>
#include <csp/arch/csp_clock.h>
#include <csp/arch/csp_system.h>

#if (CSP_DEBUG) && (CSP_USE_EXTERNAL_DEBUG == 0)

/* Custom debug function */
csp_debug_hook_func_t csp_debug_hook_func = NULL;

/* Debug levels */
bool csp_debug_level_enabled[] = {
	[CSP_ERROR]	= true,
	[CSP_WARN]	= true,
	[CSP_INFO]	= false,
	[CSP_BUFFER]	= false,
	[CSP_PACKET]	= false,
	[CSP_PROTOCOL]	= false,
	[CSP_LOCK]	= false,
};

void csp_debug_hook_set(csp_debug_hook_func_t f) {

	csp_debug_hook_func = f;
}

void do_csp_debug(csp_debug_level_t level, const char *format, ...) {

	int color = COLOR_RESET;
	va_list args;

	/* Don't print anything if log level is disabled */
	if (level > CSP_LOCK || !csp_debug_level_enabled[level])
		return;

	switch(level) {
	case CSP_INFO:
		color = COLOR_GREEN | COLOR_BOLD;
		break;
	case CSP_ERROR:
		color = COLOR_RED | COLOR_BOLD;
		break;
	case CSP_WARN:
		color = COLOR_YELLOW | COLOR_BOLD;
		break;
	case CSP_BUFFER:
		color = COLOR_MAGENTA;
		break;
	case CSP_PACKET:
		color = COLOR_GREEN;
		break;
	case CSP_PROTOCOL:
		color = COLOR_BLUE;
		break;
	case CSP_LOCK:
		color = COLOR_CYAN;
		break;
	default:
		return;
	}

	va_start(args, format);

	/* If csp_debug_hook symbol is defined, pass on the message.
	 * Otherwise, just print with pretty colors ... */
	if (csp_debug_hook_func) {
		csp_debug_hook_func(level, format, args);
	} else {
		csp_sys_set_color(color);
#if (CSP_DEBUG_TIMESTAMP)
                csp_timestamp_t ts;
                csp_clock_get_time(&ts);
                printf("%u.%06u ", ts.tv_sec, ts.tv_nsec / 1000U);
#endif
#ifdef __AVR__
		vfprintf_P(stdout, format, args);
#else
		vprintf(format, args);
#endif
		printf("\r\n");
		csp_sys_set_color(COLOR_RESET);
	}

	va_end(args);
}

void csp_debug_set_level(csp_debug_level_t level, bool value) {

	if (level <= CSP_LOCK) {
		csp_debug_level_enabled[level] = value;
	}
}

int csp_debug_get_level(csp_debug_level_t level) {

	if (level <= CSP_LOCK) {
		return csp_debug_level_enabled[level];
	}
	return 0;
}

void csp_debug_toggle_level(csp_debug_level_t level) {

	if (level <= CSP_LOCK) {
		csp_debug_level_enabled[level] = (csp_debug_level_enabled[level]) ? false : true;
	}
}

#endif // (CSP_DEBUG) && !(CSP_USE_EXTERNAL_DEBUG)
