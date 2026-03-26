#include <inttypes.h>
#include "csp_macro.h"
#include "csp/autoconfig.h"
#include "csp/csp_debug.h"

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

#include <stdarg.h>
#include <stdio.h>

#if (GOLANG)
#define CSP_PRINT_BUFFER_SIZE 1024
#else
#define CSP_PRINT_BUFFER_SIZE 512
#endif
static char csp_message_buffer[CSP_PRINT_BUFFER_SIZE];
static csp_csp_custom_print_func_t csp_custom_print_func = NULL;
static bool csp_log_level_enabled[CSP_LL_COUNT] = {true, true, true, false};

void csp_custom_print_func_default(const int log_level, const char * msg) {
	printf("%s", msg);
}

void csp_set_custom_print_func(csp_csp_custom_print_func_t cp_func) {
	csp_custom_print_func = cp_func;
}

void csp_enable_log_level(const int log_level, const bool is_enabled) {
	if (log_level >= 0 && log_level < CSP_LL_COUNT) {
		csp_log_level_enabled[log_level] = is_enabled;
		csp_print(CSP_LL_INFO, "%s log level %d\n", is_enabled ? "Enabled" : "Disabled", log_level);
	}
}

// define as __weak to enable override at link time
__weak void csp_print_func(const int log_level, const char * fmt, ...) {
	if (NULL == csp_custom_print_func) {
		return;
	}

	if (log_level >= 0 && log_level < CSP_LL_COUNT && csp_log_level_enabled[log_level]) {
		va_list args;
		va_start(args, fmt);
		vsnprintf(csp_message_buffer, CSP_PRINT_BUFFER_SIZE, fmt, args);
		va_end(args);

		csp_custom_print_func(log_level, csp_message_buffer);
	}
}

#endif // CSP_ENABLE_CSP_PRINT
