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

#ifndef _CSP_SYSTEM_H_
#define _CSP_SYSTEM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define COLOR_MASK_COLOR 	0x0F
#define COLOR_MASK_MODIFIER	0xF0

typedef enum {
	/* Colors */
	COLOR_RESET		= 0xF0,
	COLOR_BLACK		= 0x01,
	COLOR_RED		= 0x02,
	COLOR_GREEN		= 0x03,
	COLOR_YELLOW	= 0x04,
	COLOR_BLUE		= 0x05,
	COLOR_MAGENTA	= 0x06,
	COLOR_CYAN		= 0x07,
	COLOR_WHITE		= 0x08,
	/* Modifiers */
	COLOR_NORMAL	= 0x0F,
	COLOR_BOLD		= 0x10,
	COLOR_UNDERLINE	= 0x20,
	COLOR_BLINK		= 0x30,
	COLOR_HIDE		= 0x40,
} csp_color_t;

/**
 * Writes out a task list into a pre-allocate buffer,
 * use csp_sys_tasklist_size to get sizeof buffer to allocate
 * @param out pointer to output buffer
 * @return
 */
int csp_sys_tasklist(char * out);

/**
 * @return Size of tasklist buffer to allocate for the csp_sys_tasklist call
 */
int csp_sys_tasklist_size(void);

uint32_t csp_sys_memfree(void);
int csp_sys_reboot(void);
void csp_sys_set_color(csp_color_t color);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // _CSP_SYSTEM_H_
