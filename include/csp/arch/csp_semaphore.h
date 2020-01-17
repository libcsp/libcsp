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

/**
   @file

   Semaphore and Mutex interface.
*/

#include <csp/csp_platform.h>

#ifdef __cplusplus
extern "C" {
#endif

/* POSIX interface */
#if (CSP_POSIX || __DOXYGEN__)

#include <pthread.h>
#include <semaphore.h>

/**
   Semaphore (or mutex) call OK.

   @note Platform specific (this is Posix) and differs from standard CSP error codes.
*/
#define CSP_SEMAPHORE_OK 	1
/**
   Semaphore (or mutex) call failed.

   @note Platform specific (this is Posix) and differs from standard CSP error codes.
*/
#define CSP_SEMAPHORE_ERROR	2

/**
   Semaphore handle.

   @note Platform specific (this is Posix)
*/
typedef sem_t csp_bin_sem_handle_t;
/**
   Mutex handle.

   @note Platform specific (this is Posix)
*/
typedef pthread_mutex_t csp_mutex_t;

#endif // CSP_POSIX

/* MAC OS X interface */
#if (CSP_MACOSX)

#include <pthread.h>
#include "posix/pthread_queue.h"

#define CSP_SEMAPHORE_OK 	PTHREAD_QUEUE_OK
#define CSP_SEMAPHORE_ERROR 	PTHREAD_QUEUE_EMPTY

typedef pthread_queue_t * csp_bin_sem_handle_t;
typedef pthread_queue_t * csp_mutex_t;

#endif // CSP_MACOSX

#if (CSP_WINDOWS)

#include <Windows.h>

#define CSP_SEMAPHORE_OK 	1
#define CSP_SEMAPHORE_ERROR	2

typedef HANDLE csp_bin_sem_handle_t;
typedef HANDLE csp_mutex_t;

#endif // CSP_WINDOWS

/* FreeRTOS interface */
#if (CSP_FREERTOS)

#include <FreeRTOS.h>
#include <semphr.h>

#define CSP_SEMAPHORE_OK 	pdPASS
#define CSP_SEMAPHORE_ERROR	pdFAIL

typedef xSemaphoreHandle csp_bin_sem_handle_t;
typedef xSemaphoreHandle csp_mutex_t;

#endif // CSP_FREERTOS

/**
   Mutex call OK.

   @note Value is different from standard CSP error codes, see #CSP_SEMAPHORE_OK
*/
#define CSP_MUTEX_OK 		CSP_SEMAPHORE_OK

/**
   Mutex call failed.

   @note Value is different from standard CSP error codes, see #CSP_SEMAPHORE_ERROR
*/
#define CSP_MUTEX_ERROR		CSP_SEMAPHORE_ERROR

/**
   Create/initialize a mutex.

   @param[in] mutex mutex.
   @return #CSP_MUTEX_OK on success, otherwise #CSP_MUTEX_ERROR
*/
int csp_mutex_create(csp_mutex_t * mutex);

/**
   Free a mutex.

   @param[in] mutex mutex.
   @return #CSP_MUTEX_OK on success, otherwise #CSP_MUTEX_ERROR
*/
int csp_mutex_remove(csp_mutex_t * mutex);

/**
   Lock mutex.

   @param[in] mutex mutex
   @param[in] timeout timeout in mS. Use #CSP_MAX_TIMEOUT for no timeout, e.g. wait forever until locked.
   @return #CSP_MUTEX_OK on success, otherwise #CSP_MUTEX_ERROR
*/
int csp_mutex_lock(csp_mutex_t * mutex, uint32_t timeout);

/**
   Unlock mutex.

   @param[in] mutex mutex
   @return #CSP_MUTEX_OK on success, otherwise #CSP_MUTEX_ERROR
*/
int csp_mutex_unlock(csp_mutex_t * mutex);

/**
   Create/initialize a binary semaphore.

   The semaphore is created in state \a unlocked (value 1).

   On platforms supporting max values, the semaphore is created with a max value of 1, hence the naming \a binary.

   @param[in] sem semaphore
   @return #CSP_SEMAPHORE_OK on success, otherwise #CSP_SEMAPHORE_ERROR
*/
int csp_bin_sem_create(csp_bin_sem_handle_t * sem);

/**
   Free a semaphore.

   @param[in] sem semaphore.
   @return #CSP_SEMAPHORE_OK on success, otherwise #CSP_SEMAPHORE_ERROR
*/
int csp_bin_sem_remove(csp_bin_sem_handle_t * sem);

/**
   Wait/lock semaphore.

   @param[in] sem semaphore
   @param[in] timeout timeout in mS. Use #CSP_MAX_TIMEOUT for no timeout, e.g. wait forever until locked.
   @return #CSP_MUTEX_OK on success, otherwise #CSP_MUTEX_ERROR
*/
int csp_bin_sem_wait(csp_bin_sem_handle_t * sem, uint32_t timeout);

/**
   Signal/unlock semaphore.

   @param[in] sem semaphore
   @return #CSP_MUTEX_OK on success, otherwise #CSP_MUTEX_ERROR
*/
int csp_bin_sem_post(csp_bin_sem_handle_t * sem);

/**
   Signal/unlock semaphore from an ISR context.

   @param[in] sem semaphore
   @param[out] pxTaskWoken Valid reference if called from ISR, otherwise NULL!
   @return #CSP_MUTEX_OK on success, otherwise #CSP_MUTEX_ERROR*
*/
int csp_bin_sem_post_isr(csp_bin_sem_handle_t * sem, CSP_BASE_TYPE * pxTaskWoken);

#ifdef __cplusplus
}
#endif
#endif
