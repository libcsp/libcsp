

#pragma once

/**
   @file
   Debug and log.
*/

#include <inttypes.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include <csp/csp_types.h>

#if (CSP_USE_EXTERNAL_DEBUG)
/* Use external csp_debug API */
#include <csp/external/csp_debug.h>

#else 


/**
   Debug levels.
*/
typedef enum {
	CSP_ERROR	= 0, //!< Error
	CSP_WARN	= 1, //!< Warning
	CSP_INFO	= 2, //!< Informational
	CSP_BUFFER	= 3, //!< Buffer, e.g. csp_packet get/free
	CSP_PACKET	= 4, //!< Packet routing
	CSP_PROTOCOL	= 5, //!< Protocol, i.e. RDP
	CSP_LOCK	= 6, //!< Locking, i.e. semaphore
} csp_debug_level_t;

/**
   Debug level enabled/disabled.
*/
extern bool csp_debug_level_enabled[];

/**
   Extract filename component from path
*/
#define BASENAME(_file) ((strrchr(_file, '/') ? : (strrchr(_file, '\\') ? : _file)) + 1)

#if !(__DOXYGEN__)
/* Ensure defines are 'defined' */
#if !defined(CSP_DEBUG)
#define CSP_DEBUG 0
#endif

#if !defined(CSP_LOG_LEVEL_DEBUG)
#define CSP_LOG_LEVEL_DEBUG 0
#endif

#if !defined(CSP_LOG_LEVEL_INFO)
#define CSP_LOG_LEVEL_INFO 0
#endif

#if !defined(CSP_LOG_LEVEL_WARN)
#define CSP_LOG_LEVEL_WARN 0
#endif

#if !defined(CSP_LOG_LEVEL_ERROR)
#define CSP_LOG_LEVEL_ERROR 0
#endif
#endif // __DOXYGEN__

#ifdef __AVR__
        #include <stdio.h>
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
#define csp_debug(level, format, ...) { if (CSP_DEBUG && csp_debug_level_enabled[level]) do_csp_debug(level, PSTR(format), ##__VA_ARGS__); }
#else
/**
 * Log message with specific level.
 * @param level log level
 * @param format log message, printf syntax.
 */
#define csp_debug(level, format, ...) { if (CSP_DEBUG && csp_debug_level_enabled[level]) do_csp_debug(level, format, ##__VA_ARGS__); }
#endif

/**
 * Log message with level #CSP_ERROR.
 * @param format log message, printf syntax.
 */
#define csp_log_error(format, ...)    { if (CSP_LOG_LEVEL_ERROR) csp_debug(CSP_ERROR, format, ##__VA_ARGS__); }

/**
 * Log message with level #CSP_WARN.
 * @param format log message, printf syntax.
 */
#define csp_log_warn(format, ...)     { if (CSP_LOG_LEVEL_WARN) csp_debug(CSP_WARN, format, ##__VA_ARGS__); }

/**
 * Log message with level #CSP_INFO.
 * @param format log message, printf syntax.
 */
#define csp_log_info(format, ...)     { if (CSP_LOG_LEVEL_INFO) csp_debug(CSP_INFO, format, ##__VA_ARGS__); }

/**
 * Log message with level #CSP_BUFFER.
 * @param format log message, printf syntax.
 */
#define csp_log_buffer(format, ...)   { if (CSP_LOG_LEVEL_DEBUG) csp_debug(CSP_BUFFER, format, ##__VA_ARGS__); }

/**
 * Log message with level #CSP_PACKET.
 * @param format log message, printf syntax.
 */
#define csp_log_packet(format, ...)   { if (CSP_LOG_LEVEL_DEBUG) csp_debug(CSP_PACKET, format, ##__VA_ARGS__); }

/**
 * Log message with level #CSP_PROTOCOL.
 * @param format log message, printf syntax.
 */
#define csp_log_protocol(format, ...) { if (CSP_LOG_LEVEL_DEBUG) csp_debug(CSP_PROTOCOL, format, ##__VA_ARGS__); }

/**
 * Log message with level #CSP_LOCK.
 * @param format log message, printf syntax.
 */
#define csp_log_lock(format, ...)     { if (CSP_LOG_LEVEL_DEBUG) csp_debug(CSP_LOCK, format, ##__VA_ARGS__); }

/**
 * Do the actual logging (don't use directly).
 * @note Use the csp_log_<level>() macros instead.
 * @param level log level
 * @param format log message, printf syntax.
 */
void do_csp_debug(csp_debug_level_t level, const char *format, ...) __attribute__ ((format (__printf__, 2, 3)));

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
 * @param level Level to get
 * @return Level value
 */
int csp_debug_get_level(csp_debug_level_t level);

/**
 * Debug hook function.
 */
typedef void (*csp_debug_hook_func_t)(csp_debug_level_t level, const char *format, va_list args);

/**
 * Set debug/log hook function.
 */
void csp_debug_hook_set(csp_debug_hook_func_t f);

#endif // CSP_USE_EXTERNAL_DEBUG

