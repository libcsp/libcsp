

#include <csp/arch/csp_system.h>

#include <stdio.h>
#include <string.h>

#include <csp/csp.h>

int csp_sys_tasklist(char * out) {

	strcpy(out, "Tasklist not available on OSX");
	return CSP_ERR_NONE;
}

int csp_sys_tasklist_size(void) {

	return 100;
}

uint32_t csp_sys_memfree(void) {

	return 0;  // not implemented
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

	csp_print("\033[%u;%um", modifier_code, color_code);
}
