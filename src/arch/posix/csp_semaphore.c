#include <csp/arch/csp_semaphore.h>
#include <csp/csp.h>
#include <csp/csp_debug.h>

void csp_bin_sem_init(csp_bin_sem_t * sem) {
	csp_log_lock("Semaphore init: %p", sem);
	sem_init(sem, 0, 1);
}

int csp_bin_sem_wait(csp_bin_sem_t * sem, uint32_t timeout) {

	int ret;

	csp_log_lock("Wait: %p timeout %" PRIu32, sem, timeout);

	if (timeout == CSP_MAX_TIMEOUT) {
		ret = sem_wait(sem);
	} else {
		struct timespec ts;
		if (clock_gettime(CLOCK_REALTIME, &ts)) {
			return CSP_SEMAPHORE_ERROR;
		}

		uint32_t sec = timeout / 1000;
		uint32_t nsec = (timeout - 1000 * sec) * 1000000;

		ts.tv_sec += sec;

		if (ts.tv_nsec + nsec >= 1000000000) {
			ts.tv_sec++;
		}

		ts.tv_nsec = (ts.tv_nsec + nsec) % 1000000000;

		ret = sem_timedwait(sem, &ts);
	}

	if (ret != 0)
		return CSP_SEMAPHORE_ERROR;

	return CSP_SEMAPHORE_OK;
}

int csp_bin_sem_post(csp_bin_sem_t * sem) {
	csp_log_lock("Post: %p", sem);

	int value;
	sem_getvalue(sem, &value);
	if (value > 0) {
		return CSP_SEMAPHORE_OK;
	}

	if (sem_post(sem) == 0) {
		return CSP_SEMAPHORE_OK;
	}

	return CSP_SEMAPHORE_ERROR;
}
