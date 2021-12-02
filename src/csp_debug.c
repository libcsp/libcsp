#include <inttypes.h>
#include <stdarg.h>
#include <csp_autoconfig.h>

uint8_t csp_dbg_buffer_out;
uint8_t csp_dbg_errno;
uint8_t csp_dbg_conn_out;
uint8_t csp_dbg_conn_ovf;
uint8_t csp_dbg_conn_noroute;
uint8_t csp_dbg_can_errno;
uint8_t csp_dbg_inval_reply;
uint8_t csp_dbg_rdp_print;
uint8_t csp_dbg_packet_print;

#if (CSP_DEBUG)

#include <stdio.h>
__attribute__((weak)) void csp_print(const char * fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    va_end(args);
}

#else

__attribute__((weak)) void csp_print(const char * fmt, ...)) {
    return;
}

#endif