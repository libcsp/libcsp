#include <inttypes.h>
#include "csp_macro.h"
#include "csp/autoconfig.h"

uint8_t csp_dbg_buffer_out;
uint8_t csp_dbg_errno;
uint8_t csp_dbg_conn_out;
uint8_t csp_dbg_conn_ovf;
uint8_t csp_dbg_conn_noroute;
uint8_t csp_dbg_can_errno;
uint8_t csp_dbg_eth_errno;
uint8_t csp_dbg_inval_reply;
uint8_t csp_dbg_rdp_print;
uint8_t csp_dbg_packet_print;

#if (CSP_ENABLE_CSP_PRINT)
#if (CSP_PRINT_STDIO)
#include <stdarg.h>
#include <stdio.h>
__weak void csp_print_func(const char * fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}
#else
__weak void csp_print_func(const char * fmt, ...) {}
#endif
#endif
