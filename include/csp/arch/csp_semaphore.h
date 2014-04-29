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

#ifndef _CSP_SEMAPHORE_H_
#define _CSP_SEMAPHORE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include <csp/csp.h>

/* POSIX interface */
#if defined(CSP_POSIX)

#include <pthread.h>
#include <semaphore.h>

#define CSP_SEMAPHORE_OK 	1
#define CSP_SEMAPHORE_ERROR 2
#define CSP_MUTEX_OK 		CSP_SEMAPHORE_OK
#define CSP_MUTEX_ERROR		CSP_SEMAPHORE_ERROR

typedef sem_t csp_bin_sem_handle_t;
typedef pthread_mutex_t csp_mutex_t;

#endif // CSP_POSIX

/* MAC OS X interface */
#if defined(CSP_MACOSX)

#include "macosx/pthread_queue.h"

#define CSP_SEMAPHORE_OK 	PTHREAD_QUEUE_OK
#define CSP_SEMAPHORE_ERROR 	PTHREAD_QUEUE_EMPTY
#define CSP_MUTEX_OK 		CSP_SEMAPHORE_OK
#define CSP_MUTEX_ERROR		CSP_SEMAPHORE_ERROR

typedef pthread_queue_t * csp_bin_sem_handle_t;
typedef pthread_queue_t * csp_mutex_t;

#endif // CSP_POSIX

#if defined(CSP_WINDOWS)

#include <Windows.h>
#undef interface

#define CSP_SEMAPHORE_OK 	1
#define CSP_SEMAPHORE_ERROR 2
#define CSP_MUTEX_OK 		CSP_SEMAPHORE_OK
#define CSP_MUTEX_ERROR		CSP_SEMAPHORE_ERROR

typedef HANDLE csp_bin_sem_handle_t;
typedef HANDLE csp_mutex_t;

#endif

/* FreeRTOS interface */
#if defined(CSP_FREERTOS)

#include <FreeRTOS.h>
#include <semphr.h>

#define CSP_SEMAPHORE_OK 	pdPASS
#define CSP_SEMAPHORE_ERROR	pdFAIL
#define CSP_MUTEX_OK		CSP_SEMAPHORE_OK
#define CSP_MUTEX_ERROR		CSP_SEMAPHORE_ERROR

typedef xSemaphoreHandle csp_bin_sem_handle_t;
typedef xSemaphoreHandle csp_mutex_t;

#endif // CSP_FREERTOS

int csp_mutex_create(csp_mutex_t * mutex);
int csp_mutex_remove(csp_mutex_t * mutex);
int csp_mutex_lock(csp_mutex_t * mutex, uint32_t timeout);
int csp_mutex_unlock(csp_mutex_t * mutex);
int csp_bin_sem_create(csp_bin_sem_handle_t * sem);
int csp_bin_sem_remove(csp_bin_sem_handle_t * sem);
int csp_bin_sem_wait(csp_bin_sem_handle_t * sem, uint32_t timeout);
int csp_bin_sem_post(csp_bin_sem_handle_t * sem);
int csp_bin_sem_post_isr(csp_bin_sem_handle_t * sem, CSP_BASE_TYPE * task_woken);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // _CSP_SEMAPHORE_H_
