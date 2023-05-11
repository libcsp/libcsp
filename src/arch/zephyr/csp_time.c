

#include <csp/arch/csp_time.h>
#include <zephyr/kernel.h>

uint32_t csp_get_ms(void) {
	return k_uptime_get_32();
}

uint32_t csp_get_ms_isr(void) {
	return k_uptime_get_32();
}

uint32_t csp_get_s(void) {
	return k_uptime_get_32() / MSEC_PER_SEC;
}

uint32_t csp_get_s_isr(void) {
	return k_uptime_get_32() / MSEC_PER_SEC;
}
