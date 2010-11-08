/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2010 Gomspace ApS (gomspace.com)
Copyright (C) 2010 AAUSAT3 Project (aausat3.space.aau.dk) 

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

/* CSP includes */
#include <csp/csp.h>
#include <csp/csp_config.h>

#if CSP_DEBUG

static uint8_t levels_enable[] = {
		0,	// Info
		1,	// Error
		1,	// Warn
		0,	// Buffer
		0,	// Packet
		0,	// Protocol
		0	// Locks
};

/* Called by csp_debug on each execution, before calling printf */
void csp_debug_printf_hook(csp_debug_level_t level) __attribute__((weak));

/* Implement this symbol if custom formatting of debug messages is required */
void csp_debug_hook(csp_debug_level_t level, char * str) __attribute__((weak));

void csp_debug(csp_debug_level_t level, const char * format, ...) {

	va_list args;
	va_start(args, format);

	/* If csp_debug_hook symbol is defined, pass on the message.
	 * Otherwise, just print with pretty colors ... */
	if (csp_debug_hook) {
		char buf[250];
		vsnprintf(buf, 250, format, args);
		csp_debug_hook(level, buf);
	} else {
		const char * color = "";

		switch(level) {
		case CSP_INFO: 		if (!levels_enable[CSP_INFO]) return; 		color = ""; break;
		case CSP_ERROR: 	if (!levels_enable[CSP_ERROR]) return; 		color = "\E[1;31m"; break;
		case CSP_WARN: 		if (!levels_enable[CSP_WARN]) return; 		color = "\E[0;33m"; break;
		case CSP_BUFFER: 	if (!levels_enable[CSP_BUFFER]) return; 	color = "\E[0;33m"; break;
		case CSP_PACKET: 	if (!levels_enable[CSP_PACKET]) return; 	color = "\E[0;32m"; break;
		case CSP_PROTOCOL:  if (!levels_enable[CSP_PROTOCOL]) return; 	color = "\E[0;34m"; break;
		case CSP_LOCK:		if (!levels_enable[CSP_LOCK]) return; 		color = "\E[0;36m"; break;
		}

		if (csp_debug_printf_hook)
			csp_debug_printf_hook(level);

		printf("%s", color);

		printf("CSP: ");
		vprintf(format, args);

		printf("\E[0m");
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
#endif
