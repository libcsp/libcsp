/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2011 Gomspace ApS (http://www.gomspace.com)
Copyright (C) 2011 AAUSAT3 Project (http://aausat3.space.aau.dk) 

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

#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#ifdef __AVR__
#include <avr/pgmspace.h>
#endif

/* CSP includes */
#include <csp/csp.h>

#include "arch/csp_system.h"

/* Custom debug function */
csp_debug_hook_func_t csp_debug_hook_func = NULL;

/* Debug levels */
static uint8_t levels_enable[] = {
	[CSP_ERROR]		= 1,
	[CSP_WARN]		= 1,
	[CSP_INFO]		= 0,
	[CSP_BUFFER]	= 0,
	[CSP_PACKET]	= 0,
	[CSP_PROTOCOL]	= 0,
	[CSP_LOCK]		= 0,
};

/* Some compilers do not support weak symbols, so this function
 * can be used instead to set a custom debug hook */
void csp_debug_hook_set(csp_debug_hook_func_t f) {
	csp_debug_hook_func = f;
}

void do_csp_debug(csp_debug_level_t level, const char * format, ...) {

	int color = COLOR_RESET;
	va_list args;

	/* Don't print anything if log level is disabled */
	if (level > CSP_LOCK || !levels_enable[level])
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
		char buf[250];
		vsnprintf(buf, 250, format, args);
		csp_debug_hook_func(level, buf);
	} else {
		csp_sys_set_color(color);
#ifdef __AVR__
		vfprintf_P(stdout, format, args);
#else
		vprintf(format, args);
#endif
		csp_sys_set_color(COLOR_RESET);
	}

	va_end(args);

}

void csp_debug_toggle_level(csp_debug_level_t level) {
	if (level >= 7) {
		printf("Max level is 6\r\n");
		return;
	}
	levels_enable[level] = (levels_enable[level]) ? 0 : 1;
	printf("Level %u: value %u\r\n", level, levels_enable[level]);
}
