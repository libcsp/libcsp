

#include <csp/arch/csp_system.h>

#include <FreeRTOS.h>
#include <task.h>  // FreeRTOS

#include <csp/csp_debug.h>

int csp_sys_tasklist(char * out) {
#if (tskKERNEL_VERSION_MAJOR < 8)
	vTaskList((signed portCHAR *)out);
#elif (configSUPPORT_DYNAMIC_ALLOCATION == 1)
	vTaskList(out);
#endif
	return CSP_ERR_NONE;
}

int csp_sys_tasklist_size(void) {
	return 40 * uxTaskGetNumberOfTasks();
}

uint32_t csp_sys_memfree(void) {

#if (configSUPPORT_DYNAMIC_ALLOCATION == 1)
	return (uint32_t)xPortGetFreeHeapSize();
#else
	return 0;
#endif
}
