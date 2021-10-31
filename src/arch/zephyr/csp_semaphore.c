

#include <csp/arch/csp_semaphore.h>
#include <csp/csp_debug.h>

#include <zephyr.h>

static int csp_mutex_errno_to_csp(int val) {
	int ret;

	switch (val) {
		case 0:
			ret = CSP_MUTEX_OK;
			break;
		default:
			ret = CSP_MUTEX_ERROR;
			break;
	}

	return ret;
}

void csp_mutex_create_static(csp_mutex_t * mutex, csp_mutex_buffer_t * unused) {
	struct k_mutex * m = (struct k_mutex *)mutex;

	csp_log_lock("Mutex init: %p", mutex);
	(void)k_mutex_init(m);
}

int csp_mutex_lock(csp_mutex_t * mutex, uint32_t timeout) {
	int ret;
	struct k_mutex * m = (struct k_mutex *)mutex;

	csp_log_lock("Wait: %p timeout %" PRIu32, mutex, timeout);
	ret = k_mutex_lock(m, K_MSEC(timeout));

	return csp_mutex_errno_to_csp(ret);
}

int csp_mutex_unlock(csp_mutex_t * mutex) {
	int ret;
	struct k_mutex * m = (struct k_mutex *)mutex;

	ret = k_mutex_unlock(m);

	return csp_mutex_errno_to_csp(ret);
}

static int csp_bin_sem_errno_to_csp(int val) {
	int ret;

	switch (val) {
		case 0:
			ret = CSP_SEMAPHORE_OK;
			break;
		default:
			ret = CSP_SEMAPHORE_ERROR;
			break;
	}

	return ret;
}

void csp_bin_sem_create_static(csp_bin_sem_handle_t * sem, csp_bin_sem_t * unused) {
	struct k_sem * s = (struct k_sem *)sem;

	csp_log_lock("Semaphore init: %p", sem);
	(void)k_sem_init(s, 1, 1);
}

int csp_bin_sem_wait(csp_bin_sem_handle_t * sem, uint32_t timeout) {
	int ret;
	struct k_sem * s = (struct k_sem *)sem;

	csp_log_lock("Wait: %p timeout %" PRIu32, sem, timeout);
	ret = k_sem_take(s, K_MSEC(timeout));

	return csp_bin_sem_errno_to_csp(ret);
}

int csp_bin_sem_post(csp_bin_sem_handle_t * sem) {
	struct k_sem * s = (struct k_sem *)sem;

	csp_log_lock("Post: %p", sem);
	k_sem_give(s);

	return CSP_SEMAPHORE_OK;
}

int csp_bin_sem_post_isr(csp_bin_sem_handle_t * sem, int * unused) {
	ARG_UNUSED(unused);

	return csp_bin_sem_post(sem);
}
