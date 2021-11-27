

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
 * NEW DEBUG API:
 * 
 * Based on counters, and error numbers.
 * This gets rid of a lot of verbose debugging strings while
 * still maintaining the same level of debug capabilities.
 * 
 */

/* Error counters */
extern uint8_t csp_dbg_buffer_out;

extern uint8_t csp_dbg_buffer_errno;
#define CSP_DBG_BUFFER_ERR_CORRUPT_BUFFER 1
#define CSP_DBG_BUFFER_ERR_MTU_EXCEEDED 2
#define CSP_DBG_BUFFER_ERR_ALREADY_FREE 3
#define CSP_DBG_BUFFER_ERR_REFCOUNT 4

extern uint8_t csp_dbg_init_errno;
#define CSP_DBG_INIT_ERR_INVALID_CAN_CONFIG 1
#define CSP_DBG_INIT_ERR_INVALID_RTABLE_ENTRY 2

extern uint8_t csp_dbg_conn_out;
extern uint8_t csp_dbg_conn_ovf;
extern uint8_t csp_dbg_conn_noroute;
extern uint8_t csp_dbg_conn_errno;
#define CSP_DBG_CONN_ERR_UNSUPPORTED 1
#define CSP_DBG_CONN_ERR_INVALID_BIND_PORT 2
#define CSP_DBG_CONN_ERR_PORT_ALREADY_IN_USE 3
#define CSP_DBG_CONN_ERR_ALREADY_CLOSED 4
#define CSP_DBG_CONN_ERR_INVALID_POINTER 5

extern uint8_t csp_dbg_can_errno;
#define CSP_DBG_CAN_ERR_FRAME_LOST 1
#define CSP_DBG_CAN_ERR_RX_OVF 2
#define CSP_DBG_CAN_ERR_RX_OUT 3
#define CSP_DBG_CAN_ERR_SHORT_BEGIN 4
#define CSP_DBG_CAN_ERR_INCOMPLETE 5
#define CSP_DBG_CAN_ERR_UNKNOWN 6

extern uint8_t csp_dbg_inval_reply;

extern uint8_t csp_dbg_rdp_print;
extern uint8_t csp_dbg_packet_print;

#define csp_rdp_error(format, ...) { if (csp_dbg_rdp_print >= 1) printf(format, ##__VA_ARGS__); }
#define csp_rdp_protocol(format, ...) { if (csp_dbg_rdp_print >= 2) printf(format, ##__VA_ARGS__); }
#define csp_print_packet(format, ...) { if (csp_dbg_packet_print >= 1) printf(format, ##__VA_ARGS__); }


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

