#include <csp/csp_hooks.h>

__attribute__((weak)) uint32_t csp_memfree_hook(void) {
	return 0;
}

__attribute__((weak)) unsigned int csp_ps_hook(csp_packet_t * packet) {
	return 0;
}

__attribute__((weak)) void csp_reboot_hook(void) {
}

__attribute__((weak)) void csp_shutdown_hook(void) {
}
