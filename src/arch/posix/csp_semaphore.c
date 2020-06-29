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

#include <csp/arch/csp_semaphore.h>
#include <csp/csp_debug.h>

int csp_mutex_create(csp_mutex_t * mutex) {
	csp_log_lock("Mutex init: %p", mutex);
	if (pthread_mutex_init(mutex, NULL) == 0) {
		return CSP_SEMAPHORE_OK;
	}

	return CSP_SEMAPHORE_ERROR;
}

int csp_mutex_remove(csp_mutex_t * mutex) {
	if (pthread_mutex_destroy(mutex) == 0) {
		return CSP_SEMAPHORE_OK;
	}

	return CSP_SEMAPHORE_ERROR;
}

int csp_mutex_lock(csp_mutex_t * mutex, uint32_t timeout) {

	int ret;

	csp_log_lock("Wait: %p timeout %"PRIu32, mutex, timeout);

	if (timeout == CSP_MAX_TIMEOUT) {
		ret = pthread_mutex_lock(mutex);
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

		ret = pthread_mutex_timedlock(mutex, &ts);
	}

	if (ret != 0)
		return CSP_SEMAPHORE_ERROR;

	return CSP_SEMAPHORE_OK;
}

int csp_mutex_unlock(csp_mutex_t * mutex) {
	if (pthread_mutex_unlock(mutex) == 0) {
		return CSP_SEMAPHORE_OK;
	}

	return CSP_SEMAPHORE_ERROR;
}

int csp_bin_sem_create(csp_bin_sem_handle_t * sem) {
	csp_log_lock("Semaphore init: %p", sem);
	if (sem_init(sem, 0, 1) == 0) {
		return CSP_SEMAPHORE_OK;
	}

	return CSP_SEMAPHORE_ERROR;
}

int csp_bin_sem_remove(csp_bin_sem_handle_t * sem) {
	if (sem_destroy(sem) == 0) {
		return CSP_SEMAPHORE_OK;
	}

	return CSP_SEMAPHORE_ERROR;
}

int csp_bin_sem_wait(csp_bin_sem_handle_t * sem, uint32_t timeout) {

	int ret;

	csp_log_lock("Wait: %p timeout %"PRIu32, sem, timeout);

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

int csp_bin_sem_post_isr(csp_bin_sem_handle_t * sem, CSP_BASE_TYPE * task_woken) {
	if (task_woken) {
		*task_woken = 0;
	}
	return csp_bin_sem_post(sem);
}

int csp_bin_sem_post(csp_bin_sem_handle_t * sem) {
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
