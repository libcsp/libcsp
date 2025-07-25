#include <csp/csp_hooks.h>
#include <csp/csp_debug.h>

uint32_t csp_memfree_hook(void) {
	return 0;
}

unsigned int csp_ps_hook(csp_packet_t * packet) {
	return 0;
}

void csp_reboot_hook(void) {
	csp_print("Bombaclat\n");
}

void csp_shutdown_hook(void) {
	csp_print("Bombaclat\n");
}
