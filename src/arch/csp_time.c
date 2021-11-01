

#include <csp/arch/csp_time.h>

static uint32_t uptime_s_offset = 0;

uint32_t csp_get_uptime_s(void) {

	uint32_t seconds = csp_get_s();
	if (uptime_s_offset == 0) {
		uptime_s_offset = seconds;
	}
	return (seconds - uptime_s_offset);
}
