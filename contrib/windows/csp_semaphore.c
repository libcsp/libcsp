

#include "../../csp_semaphore.h"

#include <windows.h>

int csp_bin_sem_create(csp_bin_sem_handle_t * sem) {

	HANDLE semHandle = CreateSemaphore(NULL, 1, 1, NULL);
	if (semHandle == NULL) {
		return CSP_SEMAPHORE_ERROR;
	}
	*sem = semHandle;
	return CSP_SEMAPHORE_OK;
}

void csp_bin_sem_create_static(csp_bin_sem_handle_t * handle, csp_bin_sem_t * buffer) {
	csp_bin_sem_create(handle);
}

int csp_bin_sem_remove(csp_bin_sem_handle_t * sem) {

	if (!CloseHandle(*sem)) {
		return CSP_SEMAPHORE_ERROR;
	}
	return CSP_SEMAPHORE_OK;
}

int csp_bin_sem_wait(csp_bin_sem_handle_t * sem, uint32_t timeout) {

	if (WaitForSingleObject(*sem, timeout) == WAIT_OBJECT_0) {
		return CSP_SEMAPHORE_OK;
	}
	return CSP_SEMAPHORE_ERROR;
}

int csp_bin_sem_post(csp_bin_sem_handle_t * sem) {

	if (!ReleaseSemaphore(*sem, 1, NULL)) {
		return CSP_SEMAPHORE_ERROR;
	}
	return CSP_SEMAPHORE_OK;
}

int csp_bin_sem_post_isr(csp_bin_sem_handle_t * sem, int * task_woken) {

	if (task_woken != NULL) {
		*task_woken = 0;
	}
	return csp_bin_sem_post(sem);
}
