/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2012 Gomspace ApS (http://www.gomspace.com)
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

#include <pthread.h>
#include <sys/time.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>

/* CSP includes */
#include <csp/csp.h>

#include <csp/arch/csp_semaphore.h>

int csp_mutex_create(csp_mutex_t * mutex) {
	csp_log_lock("Mutex init: %p", mutex);
	*mutex = pthread_queue_create(1, sizeof(int));
	if (mutex) {
		int dummy = 0;
		pthread_queue_enqueue(*mutex, &dummy, 0);
		return CSP_SEMAPHORE_OK;
	} else {
		return CSP_SEMAPHORE_ERROR;
	}
}

int csp_mutex_remove(csp_mutex_t * mutex) {
	pthread_queue_delete(*mutex);
	return CSP_SEMAPHORE_OK;
}

int csp_mutex_lock(csp_mutex_t * mutex, uint32_t timeout) {

	int ret;
	csp_log_lock("Wait: %p timeout %"PRIu32, mutex, timeout);

	if (timeout == CSP_INFINITY) {
		/* TODO: fix this to be infinite */
		int dummy = 0;
		if (pthread_queue_dequeue(*mutex, &dummy, timeout) == PTHREAD_QUEUE_OK)
			ret = CSP_MUTEX_OK;
		else
			ret = CSP_MUTEX_ERROR;
	} else {
		int dummy = 0;
		if (pthread_queue_dequeue(*mutex, &dummy, timeout) == PTHREAD_QUEUE_OK)
			ret = CSP_MUTEX_OK;
		else
			ret = CSP_MUTEX_ERROR;
	}

	return CSP_SEMAPHORE_OK;
}

int csp_mutex_unlock(csp_mutex_t * mutex) {
	int dummy = 0;
	if (pthread_queue_enqueue(*mutex, &dummy, 0) == PTHREAD_QUEUE_OK) {
		return CSP_SEMAPHORE_OK;
	} else {
		return CSP_SEMAPHORE_ERROR;
	}
}

int csp_bin_sem_create(csp_bin_sem_handle_t * sem) {
	return csp_mutex_create(sem);
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

int csp_bin_sem_post_isr(csp_bin_sem_handle_t * sem, CSP_BASE_TYPE * task_woken) {
	return csp_mutex_unlock(sem);
}
