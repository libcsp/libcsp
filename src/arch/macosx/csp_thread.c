

#include <csp/arch/csp_thread.h>

#include <time.h>
#include <errno.h>

int csp_macosx_thread_create(void * (*routine)(void *), const char * const thread_name, unsigned int stack_size, void * parameters, unsigned int priority, pthread_t * return_handle) {

	pthread_t handle;
	int res = pthread_create(&handle, NULL, routine, parameters);
	if (res) {
		return CSP_ERR_NOMEM;
	}
	if (return_handle) {
		*return_handle = handle;
	}

	return CSP_ERR_NONE;
}

void csp_sleep_ms(unsigned int time_ms) {

	struct timespec req, rem;
	req.tv_sec = (time_ms / 1000U);
	req.tv_nsec = ((time_ms % 1000U) * 1000000U);

	while ((nanosleep(&req, &rem) < 0) && (errno == EINTR)) {
		req = rem;
	}
}
