#include <csp/csp.h>
#include <csp/arch/csp_thread.h>
#include <csp/csp_debug.h>

static void * csp_bridge(void * param) {

	/* Here there be bridging */
	while (1) {
		csp_bridge_work();
	}

	return NULL;
}

int csp_bridge_start(unsigned int task_stack_size, unsigned int task_priority, csp_iface_t * if_a, csp_iface_t * if_b) {

	(void)task_stack_size; /* We don't care the stack size on POSIX */
	(void)task_priority; /* We ignore the priority for now */

	/* Set static references to A/B side of bridge */
	csp_bridge_set_interfaces(if_a, if_b);

	static pthread_t handle;
	pthread_attr_t attributes;
	int ret;
	ret = pthread_attr_init(&attributes);
	if (ret != 0) {
		return CSP_ERR_NOMEM;
	}
	pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);

	ret = pthread_create(&handle, &attributes, csp_bridge, NULL);
	if (ret != 0) {
		csp_log_error("Failed to start task");
		return CSP_ERR_NOMEM;
	}

	return CSP_ERR_NONE;
}
