

#include <csp/arch/csp_system.h>

#include <csp/csp_debug.h>

static csp_sys_reboot_t csp_sys_reboot_callback = NULL;
static csp_sys_shutdown_t csp_sys_shutdown_callback = NULL;

void csp_sys_set_reboot(csp_sys_reboot_t reboot) {

	csp_sys_reboot_callback = reboot;
}

int csp_sys_reboot(void) {

	if (csp_sys_reboot_callback) {
		return (csp_sys_reboot_callback)();
	}
	return CSP_ERR_NOTSUP;
}

void csp_sys_set_shutdown(csp_sys_shutdown_t shutdown) {

	csp_sys_shutdown_callback = shutdown;
}

int csp_sys_shutdown(void) {

	if (csp_sys_shutdown_callback) {
		return (csp_sys_shutdown_callback)();
	}
	return CSP_ERR_NOTSUP;
}
