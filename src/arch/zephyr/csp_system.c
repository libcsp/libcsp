

#include <csp/arch/csp_system.h>
#include <csp/csp_debug.h>

int csp_sys_tasklist(char * out) {
	csp_log_warn("%s() not supported", __func__);

	return 0;
}

int csp_sys_tasklist_size(void) {
	csp_log_warn("%s() not supported", __func__);

	return 0;
}

uint32_t csp_sys_memfree(void) {
	csp_log_warn("%s() not supported", __func__);

	return 0;
}

void csp_sys_set_color(csp_color_t color) {
	/* not implemented. won't be used. */
}
