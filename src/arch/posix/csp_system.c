

#include <csp/arch/posix/csp_system.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/sysinfo.h>
#include <sys/reboot.h>
#include <linux/reboot.h>

#include <csp/csp_debug.h>

int csp_sys_tasklist(char * out) {

	strcpy(out, "Tasklist not available on POSIX");
	return CSP_ERR_NONE;
}

int csp_sys_tasklist_size(void) {

	return 100;
}

uint32_t csp_sys_memfree(void) {

	uint32_t total = 0;
	struct sysinfo info;
	sysinfo(&info);
	total = info.freeram * info.mem_unit;
	return total;
}

// helper for doing log and mapping result to CSP_ERR
static int csp_sys_log_and_return(const char * function, int res) {

	if (res != 0) {
		csp_print("%s: failed to execute, returned error: %d, errno: %d\n", function, res, errno);
		return CSP_ERR_INVAL;  // no real suitable error code
	}
	csp_print("%s: executed\n", function);
	return CSP_ERR_NONE;
}

int csp_sys_reboot_using_system(void) {

	return csp_sys_log_and_return(__FUNCTION__, system("reboot"));
}

int csp_sys_reboot_using_reboot(void) {

	sync();  // Sync filesystem
	return csp_sys_log_and_return(__FUNCTION__, reboot(LINUX_REBOOT_CMD_RESTART));
}

int csp_sys_shutdown_using_system(void) {

	return csp_sys_log_and_return(__FUNCTION__, system("halt"));
}

int csp_sys_shutdown_using_reboot(void) {

	sync();  // Sync filesystem
	return csp_sys_log_and_return(__FUNCTION__, reboot(LINUX_REBOOT_CMD_HALT));
}
