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

#ifndef _CSP_DEBUG_H_
#define _CSP_DEBUG_H_

#ifdef __cplusplus
extern "C" {
#endif

/** Debug levels */
typedef enum {
	CSP_INFO		= 0,
	CSP_ERROR	 	= 1,
	CSP_WARN	 	= 2,
	CSP_BUFFER   	= 3,
	CSP_PACKET   	= 4,
	CSP_PROTOCOL	= 5,
	CSP_LOCK	 	= 6,
} csp_debug_level_t;

/* Extract filename component from path */
#define BASENAME(_file) ((strrchr(_file, '/') ? : (strrchr(_file, '\\') ? : _file))+1)

/* Implement csp_assert_fail_action to override default failure action */
extern void __attribute__((weak)) csp_assert_fail_action(char *assertion, const char *file, int line);

#ifndef NDEBUG
	#define csp_assert(exp) 															\
	do { 																				\
		if (!(exp)) {																	\
			char *assertion = #exp;														\
			const char *file = BASENAME(__FILE__);										\
			int line = __LINE__;														\
			printf("\E[1;31m[%02"PRIu8"] Assertion \'%s\' failed in %s:%d\E[0m\r\n",	\
													my_address, assertion, file, line); \
			if (csp_assert_fail_action)													\
				csp_assert_fail_action(assertion, file, line);							\
		} 																				\
	} while (0)
#else
	#define csp_assert(...) do {} while (0)
#endif

#ifdef CSP_DEBUG
	#define csp_debug(level, format, ...) csp_debug_ex(level, "[%02"PRIu8"] %s:%d " format, my_address, BASENAME(__FILE__), __LINE__, ##__VA_ARGS__)
	void csp_debug_ex(csp_debug_level_t level, const char * format, ...);
#else
	#define csp_debug(...) do {} while (0)
	#define csp_debug_toggle_level(...) do {} while (0)
	#define csp_route_print_interfaces(...) do {} while (0)
	#define csp_route_print_table(...) do {} while (0)
	#define csp_conn_print_table(...) do {} while (0)
	#define csp_buffer_print_table(...) do {} while (0)
	#define csp_debug_hook_set(...) do {} while (0)
#endif

/* Quick and dirty hack to place AVR debug info in progmem */
#if defined(CSP_DEBUG) && defined(__AVR__)
	#include <avr/pgmspace.h>
	#undef csp_debug
	#define csp_debug(level, format, ...) printf_P(PSTR(format), ##__VA_ARGS__)
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // _CSP_DEBUG_H_
