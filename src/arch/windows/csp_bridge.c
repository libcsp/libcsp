#include <csp/csp.h>
#include <csp/csp_debug.h>
#include <process.h>

static unsigned int __attribute__((stdcall)) csp_bridge(void * param) {

	/* Here there be bridging */
	while (1) {
		csp_bridge_work();
	}

	return 0;
}

int csp_bridge_start(unsigned int task_stack_size, unsigned int task_priority, csp_iface_t * if_a, csp_iface_t * if_b) {

	/* Set static references to A/B side of bridge */
	csp_bridge_set_interfaces(if_a, if_b);

	uintptr_t ret = _beginthreadex(NULL, 0, csp_bridge, NULL, 0, NULL);
	if (ret == 0) {
		csp_log_error("Failed to start task");
		return CSP_ERR_NOMEM;
	}

	return CSP_ERR_NONE;
}
