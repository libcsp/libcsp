#include <csp/csp_hooks.h>

#include <unistd.h>
#include <sys/sysinfo.h>
#include <sys/reboot.h>
#include <linux/reboot.h>

uint32_t csp_memfree_hook(void) {
	uint32_t total = 0;
	struct sysinfo info;
	sysinfo(&info);
	total = info.freeram * info.mem_unit;
	return total;
}

unsigned int csp_ps_hook(csp_packet_t * packet) {
	return 0;
}

void csp_reboot_hook(void) {
	sync();
	reboot(LINUX_REBOOT_CMD_RESTART);
}

void csp_shutdown_hook(void) {
	sync();
	reboot(LINUX_REBOOT_CMD_HALT);
}
