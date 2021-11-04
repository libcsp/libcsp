#include <csp/arch/csp_thread.h>

#include <process.h>

int csp_windows_thread_create(unsigned int (*routine)(void *), const char * const thread_name, unsigned int stack_size, void * parameters, unsigned int priority, HANDLE * return_handle) {

	HANDLE handle = (HANDLE)_beginthreadex(NULL, 0, routine, parameters, 0, 0);
	if (handle == 0) {
		return CSP_ERR_NOMEM;
	}
	if (return_handle) {
		*return_handle = handle;
	}

	return CSP_ERR_NONE;
}

void csp_sleep_ms(unsigned int time_ms) {

	Sleep(time_ms);
}
