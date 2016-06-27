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

#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#ifdef __AVR__
#include <avr/pgmspace.h>
#endif

/* CSP includes */
#include <csp/csp.h>

#define COLOR_MASK_COLOR	0x0F
#define COLOR_MASK_MODIFIER	0xF0

typedef enum {
	/* Colors */
	COLOR_RESET	= 0xF0,
	COLOR_BLACK	= 0x01,
	COLOR_RED	= 0x02,
	COLOR_GREEN	= 0x03,
	COLOR_YELLOW	= 0x04,
	COLOR_BLUE	= 0x05,
	COLOR_MAGENTA	= 0x06,
	COLOR_CYAN	= 0x07,
	COLOR_WHITE	= 0x08,
	/* Modifiers */
	COLOR_NORMAL	= 0x0F,
	COLOR_BOLD	= 0x10,
	COLOR_UNDERLINE	= 0x20,
	COLOR_BLINK	= 0x30,
	COLOR_HIDE	= 0x40,
} csp_color_t;

/* Custom debug function */
csp_debug_hook_func_t csp_debug_hook_func = NULL;

/* Debug levels */
static bool csp_debug_level_enabled[] = {
	[CSP_ERROR]	= true,
	[CSP_WARN]	= true,
	[CSP_INFO]	= false,
	[CSP_BUFFER]	= false,
	[CSP_PACKET]	= false,
	[CSP_PROTOCOL]	= false,
	[CSP_LOCK]	= false,
};

/* Some compilers do not support weak symbols, so this function
 * can be used instead to set a custom debug hook */
void csp_debug_hook_set(csp_debug_hook_func_t f)
{
	csp_debug_hook_func = f;
}

void csp_debug_set_color(csp_color_t color)
{
	unsigned int color_code, modifier_code;
	switch (color & COLOR_MASK_COLOR) {
		case COLOR_BLACK:
			color_code = 30; break;
		case COLOR_RED:
			color_code = 31; break;
		case COLOR_GREEN:
			color_code = 32; break;
		case COLOR_YELLOW:
			color_code = 33; break;
		case COLOR_BLUE:
			color_code = 34; break;
		case COLOR_MAGENTA:
			color_code = 35; break;
		case COLOR_CYAN:
			color_code = 36; break;
		case COLOR_WHITE:
			color_code = 37; break;
		case COLOR_RESET:
		default:
			color_code = 0; break;
	}

	switch (color & COLOR_MASK_MODIFIER) {
		case COLOR_BOLD:
			modifier_code = 1; break;
		case COLOR_UNDERLINE:
			modifier_code = 2; break;
		case COLOR_BLINK:
			modifier_code = 3; break;
		case COLOR_HIDE:
			modifier_code = 4; break;
		case COLOR_NORMAL:
		default:
			modifier_code = 0; break;
	}

	printf("\033[%u;%um", modifier_code, color_code);
}

void do_csp_debug(csp_debug_level_t level, const char *format, ...)
{
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
		csp_debug_set_color(color);
#ifdef __AVR__
		vfprintf_P(stdout, format, args);
#else
		vprintf(format, args);
#endif
		printf("\r\n");
		csp_debug_set_color(COLOR_RESET);
	}

	va_end(args);
}

void csp_debug_set_level(csp_debug_level_t level, bool value)
{
	if (level > CSP_LOCK)
		return;
	csp_debug_level_enabled[level] = value;
}

int csp_debug_get_level(csp_debug_level_t level)
{
	if (level > CSP_LOCK)
		return 0;
	return csp_debug_level_enabled[level];
}

void csp_debug_toggle_level(csp_debug_level_t level)
{
	if (level > CSP_LOCK) {
		printf("Max level is 6\r\n");
		return;
	}
	csp_debug_level_enabled[level] = (csp_debug_level_enabled[level]) ? false : true;
	printf("Level %u: value %u\r\n", level, csp_debug_level_enabled[level]);
}
