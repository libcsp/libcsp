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

#ifndef _CSP_THREAD_H_
#define _CSP_THREAD_H_

/**
   @file

   Thread (task) interface.
*/

#include <csp/csp_platform.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
  POSIX interface
*/
#if (CSP_POSIX || CSP_MACOSX || __DOXYGEN__)

#include <pthread.h>

/**
   Platform specific thread handle.
*/
typedef pthread_t csp_thread_handle_t;

/**
   Platform specific thread return type.
*/
typedef void * csp_thread_return_t;

/**
   Platform specific thread function.
   @param[in] parameter parameter to thread function #csp_thread_return_t.
*/
typedef csp_thread_return_t (* csp_thread_func_t)(void * parameter);

/**
   Macro for creating a thread.
*/
#define CSP_DEFINE_TASK(task_name) csp_thread_return_t task_name(void * param)

/**
   Return value for a thread function.
   Can be used as argument for normal return, eg "return CSP_TASK_RETURN";
*/
#define CSP_TASK_RETURN NULL

#endif // CSP_POSIX

/*
  Windows interface
*/
#if (CSP_WINDOWS)

#include <Windows.h>

typedef HANDLE csp_thread_handle_t;
typedef unsigned int csp_thread_return_t;
typedef csp_thread_return_t (* csp_thread_func_t)(void *) __attribute__((stdcall));

#define CSP_DEFINE_TASK(task_name) csp_thread_return_t __attribute__((stdcall)) task_name(void * param) 
#define CSP_TASK_RETURN 0

#endif // CSP_WINDOWS

/*
  FreeRTOS interface
*/
#if (CSP_FREERTOS)

#include <FreeRTOS.h>
#include <task.h>

typedef xTaskHandle csp_thread_handle_t;
typedef void csp_thread_return_t;
typedef csp_thread_return_t (* csp_thread_func_t)(void *);

#define CSP_DEFINE_TASK(task_name) csp_thread_return_t task_name(void * param)
#define CSP_TASK_RETURN

#endif // CSP_FREERTOS

/**
   Create thread (task).

   @param[in] func thread function
   @param[in] name name of thread, supported on: FreeRTOS.
   @param[in] stack_size stack size, supported on: posix (bytes), FreeRTOS (words, word = 4 bytes).
   @param[in] parameter parameter for thread function.
   @param[in] priority thread priority, supported on: FreeRTOS.
   @param[out] handle reference to created thread.
   @return #CSP_ERR_NONE on success, otherwise an error code.
*/
int csp_thread_create(csp_thread_func_t func, const char * const name, unsigned int stack_size, void * parameter, unsigned int priority, csp_thread_handle_t * handle);

/**
   Exit current thread.
   @note Not supported on all platforms.
*/
void csp_thread_exit(void);

/**
   Sleep X mS.
   @param[in] time_ms mS to sleep.
*/
void csp_sleep_ms(unsigned int time_ms);

#ifdef __cplusplus
}
#endif
#endif
