

#include <csp/arch/csp_system.h>

#include <string.h>
#include <windows.h>

#include <csp/csp_debug.h>

int csp_sys_tasklist(char * out) {

	strcpy(out, "Tasklist not available on Windows");
	return CSP_ERR_NONE;
}

int csp_sys_tasklist_size(void) {

	return 100;
}

uint32_t csp_sys_memfree(void) {

	MEMORYSTATUSEX statex;
	statex.dwLength = sizeof(statex);
	GlobalMemoryStatusEx(&statex);
	DWORDLONG freePhysicalMem = statex.ullAvailPhys;
	size_t total = (size_t)freePhysicalMem;
	return (uint32_t)total;
}
