#pragma once

#include <inttypes.h>
#include <stdio.h>

/**
 * NEW DEBUG API:
 * 
 * Based on counters, and error numbers.
 * This gets rid of a lot of verbose debugging strings while
 * still maintaining the same level of debug capabilities.
 * 
 */

/** Error counters */
extern uint8_t csp_dbg_buffer_out;
extern uint8_t csp_dbg_conn_out;
extern uint8_t csp_dbg_conn_ovf;
extern uint8_t csp_dbg_conn_noroute;
extern uint8_t csp_dbg_inval_reply;

/* Central errno */
extern uint8_t csp_dbg_errno;
#define CSP_DBG_BUFFER_ERR_CORRUPT_BUFFER 1
#define CSP_DBG_BUFFER_ERR_MTU_EXCEEDED 2
#define CSP_DBG_BUFFER_ERR_ALREADY_FREE 3
#define CSP_DBG_BUFFER_ERR_REFCOUNT 4
#define CSP_DBG_INIT_ERR_INVALID_CAN_CONFIG 5
#define CSP_DBG_INIT_ERR_INVALID_RTABLE_ENTRY 6
#define CSP_DBG_CONN_ERR_UNSUPPORTED 7
#define CSP_DBG_CONN_ERR_INVALID_BIND_PORT 8
#define CSP_DBG_CONN_ERR_PORT_ALREADY_IN_USE 9
#define CSP_DBG_CONN_ERR_ALREADY_CLOSED 10
#define CSP_DBG_CONN_ERR_INVALID_POINTER 11

/* CAN protocol specific errno */
extern uint8_t csp_dbg_can_errno;
#define CSP_DBG_CAN_ERR_FRAME_LOST 1
#define CSP_DBG_CAN_ERR_RX_OVF 2
#define CSP_DBG_CAN_ERR_RX_OUT 3
#define CSP_DBG_CAN_ERR_SHORT_BEGIN 4
#define CSP_DBG_CAN_ERR_INCOMPLETE 5
#define CSP_DBG_CAN_ERR_UNKNOWN 6

/* Toogle flags for rdp and packet print */
extern uint8_t csp_dbg_rdp_print;
extern uint8_t csp_dbg_packet_print;

/* Helper macros for toggled printf */
#define csp_rdp_error(format, ...) { if (csp_dbg_rdp_print >= 1) printf(format, ##__VA_ARGS__); }
#define csp_rdp_protocol(format, ...) { if (csp_dbg_rdp_print >= 2) printf(format, ##__VA_ARGS__); }
#define csp_print_packet(format, ...) { if (csp_dbg_packet_print >= 1) printf(format, ##__VA_ARGS__); }
