

#include "../../csp_semaphore.h"

#include <sys/time.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <csp/csp_debug.h>



int csp_bin_sem_create(csp_bin_sem_handle_t * sem) {
	//csp_print("Mutex init: %p\n", (void *)sem);
	*sem = pthread_queue_create(1, sizeof(int));
	if (*sem) {
		int dummy = 0;
		pthread_queue_enqueue(*sem, &dummy, 0);
		return CSP_SEMAPHORE_OK;
	}

	return CSP_SEMAPHORE_ERROR;
}

void csp_bin_sem_create_static(csp_bin_sem_handle_t * handle, csp_bin_sem_t * buffer) {
	csp_bin_sem_create(handle);
}

int csp_bin_sem_remove(csp_bin_sem_handle_t * sem) {
	pthread_queue_delete(*sem);
	return CSP_SEMAPHORE_OK;
}

int csp_bin_sem_wait(csp_bin_sem_handle_t * sem, uint32_t timeout) {

	//csp_print("Wait: %p timeout %" PRIu32"\n", (void *)sem, timeout);

	int dummy = 0;
	if (pthread_queue_dequeue(*sem, &dummy, timeout) == PTHREAD_QUEUE_OK) {
		return CSP_SEMAPHORE_OK;
	}

	return CSP_SEMAPHORE_ERROR;
}

int csp_bin_sem_post(csp_bin_sem_handle_t * sem) {
	int dummy = 0;
	if (pthread_queue_enqueue(*sem, &dummy, 0) == PTHREAD_QUEUE_OK) {
		return CSP_SEMAPHORE_OK;
	}
	return CSP_SEMAPHORE_ERROR;

}

int csp_bin_sem_post_isr(csp_bin_sem_handle_t * sem, int * task_woken) {
	return csp_mutex_unlock(sem);
}
