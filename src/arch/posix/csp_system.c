#include <csp/csp_hooks.h>
#include "csp_macro.h"

#include <unistd.h>
#include <sys/sysinfo.h>
#ifdef __CYGWIN__
#include <csp/csp_debug.h>
#else
#include <sys/reboot.h>
#include <linux/reboot.h>
#endif

__weak uint32_t csp_memfree_hook(void) {
	uint32_t total = 0;
	struct sysinfo info;
	sysinfo(&info);
	total = info.freeram * info.mem_unit;
	return total;
}

__weak unsigned int csp_ps_hook(csp_packet_t * packet) {
	(void)packet; /* Avoid compiler warnings about unused parameter */
	return 0;
}

__weak void csp_reboot_hook(void) {
#ifdef __CYGWIN__
    csp_print("HALTED - Please reboot\n");
    while (true)
        sleep(1);
#else
	sync();
	reboot(LINUX_REBOOT_CMD_RESTART);
#endif
}

__weak void csp_shutdown_hook(void) {
#ifdef __CYGWIN__
    csp_print("HALTED - Please power off\n");
    while (true)
        sleep(1);
#else
	sync();
	reboot(LINUX_REBOOT_CMD_HALT);
#endif
}
