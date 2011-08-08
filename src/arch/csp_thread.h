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

#ifndef _CSP_THREAD_H_
#define _CSP_THREAD_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <csp/csp.h>

/* POSIX interface */
#if defined(_CSP_POSIX_)

#include <pthread.h>

#define csp_thread_exit() pthread_exit(NULL)

typedef pthread_t csp_thread_handle_t;
typedef void* csp_thread_return_t;

#endif // _CSP_POSIX_

/* FreeRTOS interface */
#if defined(_CSP_FREERTOS_)

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define csp_thread_exit() vTaskDelete(NULL)

typedef xTaskHandle csp_thread_handle_t;
typedef void csp_thread_return_t;

#endif // _CSP_FREERTOS_

int csp_thread_create(csp_thread_return_t (* routine)(void *), const signed char * const thread_name, unsigned short stack_depth, void * parameters, unsigned int priority, csp_thread_handle_t * handle);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // _CSP_THREAD_H_
