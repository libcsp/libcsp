#include <csp/csp_hooks.h>

#include <windows.h>

uint32_t csp_memfree_hook(void) {
	MEMORYSTATUSEX statex;
	statex.dwLength = sizeof(statex);
	if (GlobalMemoryStatusEx(&statex)) {
		return statex.ullAvailPhys;
	}
	return 0;
}

unsigned int csp_ps_hook(csp_packet_t * packet) {
	return 0;
}

void csp_reboot_hook(void) {
}

void csp_shutdown_hook(void) {
}
