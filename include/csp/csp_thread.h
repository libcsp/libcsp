/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2010 Gomspace ApS (gomspace.com)
Copyright (C) 2010 AAUSAT3 Project (aausat3.space.aau.dk) 

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

#include <stdint.h>
#include <csp/csp.h>

/* POSIX interface */
#if defined(__CSP_POSIX__)

#include <pthread.h>

#define csp_thread_exit() pthread_exit(NULL)

typedef pthread_t csp_thread_handle_t;
typedef void* csp_thread_return_t;

#endif // __CSP_POSIX__

/* FreeRTOS interface */
#if defined(__CSP_FREERTOS__)

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define csp_thread_exit() return

typedef xTaskHandle csp_thread_handle_t;
typedef void csp_thread_return_t;

#endif // __CSP_FREERTOS__

#endif // _CSP_THREAD_H_
