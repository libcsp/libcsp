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

#ifndef _CSP_DEBUG_H_
#define _CSP_DEBUG_H_

#include <inttypes.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Debug levels */
typedef enum {
	CSP_ERROR	= 0,
	CSP_WARN	= 1,
	CSP_INFO	= 2,
	CSP_BUFFER	= 3,
	CSP_PACKET	= 4,
	CSP_PROTOCOL	= 5,
	CSP_LOCK	= 6,
} csp_debug_level_t;

/* Extract filename component from path */
#define BASENAME(_file) ((strrchr(_file, '/') ? : (strrchr(_file, '\\') ? : _file)) + 1)

/* Implement csp_assert_fail_action to override default failure action */
extern void __attribute__((weak)) csp_assert_fail_action(char *assertion, const char *file, int line);

#ifndef NDEBUG
	#define csp_assert(exp)										\
	do {												\
		if (!(exp)) {										\
			char *assertion = #exp;								\
			const char *file = BASENAME(__FILE__);						\
			int line = __LINE__;								\
			printf("\E[1;31m[%02" PRIu8 "] Assertion \'%s\' failed in %s:%d\E[0m\r\n",	\
			       csp_get_address(), assertion, file, line);				\
			if (csp_assert_fail_action)							\
				csp_assert_fail_action(assertion, file, line);				\
		}											\
	} while (0)
#else
	#define csp_assert(...) do {} while (0)
#endif

#ifdef __AVR__
	#include <avr/pgmspace.h>
	#define CONSTSTR(data) PSTR(data)
	#undef printf
	#undef sscanf
	#undef scanf
	#undef sprintf
	#undef snprintf
	#define printf(s, ...) printf_P(PSTR(s), ## __VA_ARGS__)
	#define sscanf(buf, s, ...) sscanf_P(buf, PSTR(s), ## __VA_ARGS__)
	#define scanf(s, ...) scanf_P(PSTR(s), ## __VA_ARGS__)
	#define sprintf(buf, s, ...) sprintf_P(buf, PSTR(s), ## __VA_ARGS__)
	#define snprintf(buf, size, s, ...) snprintf_P(buf, size, PSTR(s), ## __VA_ARGS__)
#else
	#define CONSTSTR(data) data
#endif

#ifdef CSP_DEBUG
	#define csp_debug(level, format, ...) do { do_csp_debug(level, CONSTSTR(format), ##__VA_ARGS__); } while(0)
#else
	#define csp_debug(...) do {} while (0)
#endif

#ifdef CSP_LOG_LEVEL_ERROR
	#define csp_log_error(format, ...) csp_debug(CSP_ERROR, format, ##__VA_ARGS__)
#else
	#define csp_log_error(...) do {} while (0)
#endif

#ifdef CSP_LOG_LEVEL_WARN
	#define csp_log_warn(format, ...) csp_debug(CSP_WARN, format, ##__VA_ARGS__)
#else
	#define csp_log_warn(...) do {} while (0)
#endif

#ifdef CSP_LOG_LEVEL_INFO
	#define csp_log_info(format, ...) csp_debug(CSP_INFO, format, ##__VA_ARGS__)
#else
	#define csp_log_info(...) do {} while (0)
#endif

#ifdef CSP_LOG_LEVEL_DEBUG
	#define csp_log_buffer(format, ...) csp_debug(CSP_BUFFER, format, ##__VA_ARGS__)
	#define csp_log_packet(format, ...) csp_debug(CSP_PACKET, format, ##__VA_ARGS__)
	#define csp_log_protocol(format, ...) csp_debug(CSP_PROTOCOL, format, ##__VA_ARGS__)
	#define csp_log_lock(format, ...) csp_debug(CSP_LOCK, format, ##__VA_ARGS__)
#else
	#define csp_log_buffer(...) do {} while (0)
	#define csp_log_packet(...) do {} while (0)
	#define csp_log_protocol(...) do {} while (0)
	#define csp_log_lock(...) do {} while (0)
#endif

/**
 * This function should not be used directly, use csp_log_<level>() macro instead
 * @param level
 * @param format
 */
void do_csp_debug(csp_debug_level_t level, const char *format, ...);

/**
 * Toggle debug level on/off
 * @param level Level to toggle
 */
void csp_debug_toggle_level(csp_debug_level_t level);

/**
 * Set debug level
 * @param level Level to set
 * @param value New level value
 */
void csp_debug_set_level(csp_debug_level_t level, bool value);

/**
 * Get current debug level value
 * @param level Level value to get
 * @return Level value
 */
int csp_debug_get_level(csp_debug_level_t level);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // _CSP_DEBUG_H_
