#include <zephyr.h>

void csp_sleep_ms(unsigned int time_ms) {
	k_sleep(K_MSEC(time_ms));
}
