#include <csp/csp_hooks.h>

#include <unistd.h>

uint32_t csp_memfree_hook(void) {
	uint32_t total = 0;
	return total;
}

unsigned int csp_ps_hook(csp_packet_t * packet) {
	return 0;
}

void csp_reboot_hook(void) {
	sync();
}

void csp_shutdown_hook(void) {
	sync();
}
