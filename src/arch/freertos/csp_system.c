

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

void csp_sys_set_color(csp_color_t color) {

	unsigned int color_code, modifier_code;
	switch (color & COLOR_MASK_COLOR) {
		case COLOR_BLACK:
			color_code = 30;
			break;
		case COLOR_RED:
			color_code = 31;
			break;
		case COLOR_GREEN:
			color_code = 32;
			break;
		case COLOR_YELLOW:
			color_code = 33;
			break;
		case COLOR_BLUE:
			color_code = 34;
			break;
		case COLOR_MAGENTA:
			color_code = 35;
			break;
		case COLOR_CYAN:
			color_code = 36;
			break;
		case COLOR_WHITE:
			color_code = 37;
			break;
		case COLOR_RESET:
		default:
			color_code = 0;
			break;
	}

	switch (color & COLOR_MASK_MODIFIER) {
		case COLOR_BOLD:
			modifier_code = 1;
			break;
		case COLOR_UNDERLINE:
			modifier_code = 2;
			break;
		case COLOR_BLINK:
			modifier_code = 3;
			break;
		case COLOR_HIDE:
			modifier_code = 4;
			break;
		case COLOR_NORMAL:
		default:
			modifier_code = 0;
			break;
	}

	printf("\033[%u;%um", modifier_code, color_code);
}
