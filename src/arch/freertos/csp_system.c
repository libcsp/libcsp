#include <csp/csp_hooks.h>

#include <FreeRTOS.h>
#include <task.h>

#if (configSUPPORT_DYNAMIC_ALLOCATION == 1)
uint32_t csp_memfree_hook(void) {
	return (uint32_t)xPortGetFreeHeapSize();
}
#endif
