/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2011 Gomspace ApS (http://www.gomspace.com)
Copyright (C) 2011 AAUSAT3 Project (http://aausat3.space.aau.dk) 

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
#if defined(_CSP_POSIX_)

#include <pthread.h>
#include <semaphore.h>

#define CSP_SEMAPHORE_OK 1
#define CSP_SEMAPHORE_ERROR 2

typedef sem_t csp_bin_sem_handle_t;
typedef pthread_mutex_t csp_mutex_t;

#endif // _CSP_POSIX_

/* FreeRTOS interface */
#if defined(_CSP_FREERTOS_)

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#define CSP_SEMAPHORE_OK pdPASS
#define CSP_SEMAPHORE_ERROR pdFAIL

typedef xSemaphoreHandle csp_bin_sem_handle_t;
typedef xSemaphoreHandle csp_mutex_t;

#endif // _CSP_FREERTOS_

int csp_mutex_create(csp_mutex_t * mutex);
int csp_mutex_remove(csp_mutex_t * mutex);
int csp_mutex_lock(csp_mutex_t * mutex, int timeout);
int csp_mutex_unlock(csp_mutex_t * mutex);
int csp_bin_sem_create(csp_bin_sem_handle_t * sem);
int csp_bin_sem_remove(csp_bin_sem_handle_t * sem);
int csp_bin_sem_wait(csp_bin_sem_handle_t * sem, int timeout);
int csp_bin_sem_post(csp_bin_sem_handle_t * sem);
int csp_bin_sem_post_isr(csp_bin_sem_handle_t * sem, CSP_BASE_TYPE * task_woken);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // _CSP_SEMAPHORE_H_
