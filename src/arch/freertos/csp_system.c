#include <csp/csp_hooks.h>
#include "csp_macro.h"

#include <FreeRTOS.h>
#include <task.h>

__weak uint32_t csp_memfree_hook(void) {
#if (configSUPPORT_DYNAMIC_ALLOCATION == 1)
	return (uint32_t)xPortGetFreeHeapSize();
#else
	return 0;
#endif
}

__weak unsigned int csp_ps_hook(csp_packet_t * packet) {
	return 0;
}
