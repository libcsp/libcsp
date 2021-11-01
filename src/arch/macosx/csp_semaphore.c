

#include <csp/arch/csp_semaphore.h>

#include <sys/time.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <csp/csp_debug.h>

int csp_mutex_create(csp_mutex_t * mutex) {
	csp_log_lock("Mutex init: %p", mutex);
	*mutex = pthread_queue_create(1, sizeof(int));
	if (*mutex) {
		int dummy = 0;
		pthread_queue_enqueue(*mutex, &dummy, 0);
		return CSP_SEMAPHORE_OK;
	}

	return CSP_SEMAPHORE_ERROR;
}

void csp_mutex_create_static(csp_mutex_t * handle, csp_mutex_buffer_t * buffer) {
	csp_mutex_create(handle);
}

int csp_mutex_remove(csp_mutex_t * mutex) {
	pthread_queue_delete(*mutex);
	return CSP_SEMAPHORE_OK;
}

int csp_mutex_lock(csp_mutex_t * mutex, uint32_t timeout) {

	csp_log_lock("Wait: %p timeout %" PRIu32, mutex, timeout);

	int dummy = 0;
	if (pthread_queue_dequeue(*mutex, &dummy, timeout) == PTHREAD_QUEUE_OK) {
		return CSP_SEMAPHORE_OK;
	}

	return CSP_SEMAPHORE_ERROR;
}

int csp_mutex_unlock(csp_mutex_t * mutex) {
	int dummy = 0;
	if (pthread_queue_enqueue(*mutex, &dummy, 0) == PTHREAD_QUEUE_OK) {
		return CSP_SEMAPHORE_OK;
	}
	return CSP_SEMAPHORE_ERROR;
}

int csp_bin_sem_create(csp_bin_sem_handle_t * sem) {
	return csp_mutex_create(sem);
}

void csp_bin_sem_create_static(csp_bin_sem_handle_t * handle, csp_bin_sem_t * buffer) {
	csp_bin_sem_create(handle);
}

int csp_bin_sem_remove(csp_bin_sem_handle_t * sem) {
	return csp_mutex_remove(sem);
}

int csp_bin_sem_wait(csp_bin_sem_handle_t * sem, uint32_t timeout) {
	return csp_mutex_lock(sem, timeout);
}

int csp_bin_sem_post(csp_bin_sem_handle_t * sem) {
	return csp_mutex_unlock(sem);
}

int csp_bin_sem_post_isr(csp_bin_sem_handle_t * sem, int * task_woken) {
	return csp_mutex_unlock(sem);
}
