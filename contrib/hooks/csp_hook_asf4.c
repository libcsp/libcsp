/* Atmel Software Framework 4 */
#include <hal_sleep.h>

void csp_reboot_hook(void) {
    NVIC_SystemReset();
}

void csp_shutdown_hook(void) {
    NVIC_SystemReset();
}
