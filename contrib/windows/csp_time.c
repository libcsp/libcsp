

#include <csp/arch/csp_time.h>

#include <windows.h>

uint32_t csp_get_ms(void) {

	return (uint32_t)GetTickCount();
}

uint32_t csp_get_ms_isr(void) {

	return csp_get_ms();
}

uint32_t csp_get_s(void) {

	uint32_t time_ms = csp_get_ms();
	return time_ms / 1000;
}

uint32_t csp_get_s_isr(void) {

	return csp_get_s();
}
