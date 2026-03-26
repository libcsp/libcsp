/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2012 GomSpace ApS (http://www.gomspace.com)
Copyright (C) 2012 AAUSAT3 Project (http://aausat3.space.aau.dk)

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "../../csp_semaphore.h"
#include "csp/csp_debug.h"

void csp_bin_sem_init(csp_bin_sem_t * sem) {

	HANDLE semHandle = CreateSemaphore(NULL, 1, 1, NULL);
	if (semHandle == NULL) {
		csp_print(CSP_LL_ERROR, "Error creating windows semaphore");
	}
	*sem = semHandle;
}

int csp_bin_sem_remove(csp_bin_sem_t * sem) {

	if (!CloseHandle(*sem)) {
		return CSP_SEMAPHORE_ERROR;
	}
	return CSP_SEMAPHORE_OK;
}

int csp_bin_sem_wait(csp_bin_sem_t * sem, unsigned int timeout) {

	if (WaitForSingleObject(*sem, timeout) == WAIT_OBJECT_0) {
		return CSP_SEMAPHORE_OK;
	}
	return CSP_SEMAPHORE_ERROR;
}

int csp_bin_sem_post(csp_bin_sem_t * sem) {

	if (!ReleaseSemaphore(*sem, 1, NULL)) {
		return CSP_SEMAPHORE_ERROR;
	}
	return CSP_SEMAPHORE_OK;
}

int csp_bin_sem_post_isr(csp_bin_sem_t * sem, int * task_woken) {

	if (task_woken != NULL) {
		*task_woken = 0;
	}
	return csp_bin_sem_post(sem);
}
