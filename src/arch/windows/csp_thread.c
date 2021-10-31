

#include <csp/arch/csp_thread.h>

#include <process.h>

int csp_thread_create(csp_thread_func_t routine, const char * const thread_name, unsigned int stack_size, void * parameters, unsigned int priority, csp_thread_handle_t * return_handle) {

	HANDLE handle = (HANDLE)_beginthreadex(NULL, 0, routine, parameters, 0, 0);
	if (handle == 0) {
		return CSP_ERR_NOMEM;
	}
	if (return_handle) {
		*return_handle = handle;
	}

	return CSP_ERR_NONE;
}

void csp_thread_exit(void) {

	_endthreadex(CSP_TASK_RETURN);
}

void csp_sleep_ms(unsigned int time_ms) {

	Sleep(time_ms);
}
