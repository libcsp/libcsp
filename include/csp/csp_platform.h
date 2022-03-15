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

#ifndef _CSP_PLATFORM_H_
#define _CSP_PLATFORM_H_

/**
   @file
   Platform support.
*/

#include <csp/csp_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Set OS */
#if (CSP_POSIX || CSP_WINDOWS || CSP_MACOSX || __DOXYGEN__)
	/** Base type. Mainly used for FreeRTOS calls to trigger task re-scheduling. */
	#define CSP_BASE_TYPE int
	/** Max timeout time. On platforms supporting no timeouts (e.g. Linux), the timeout will be converted to \a forever. */
	#define CSP_MAX_TIMEOUT (UINT32_MAX)
	/** Declare critical lock. */
	#define CSP_DEFINE_CRITICAL(lock) static csp_bin_sem_handle_t lock
	/** Initialize critical lock. */
	#define CSP_INIT_CRITICAL(lock) ({(csp_bin_sem_create(&lock) == CSP_SEMAPHORE_OK) ? CSP_ERR_NONE : CSP_ERR_NOMEM;})
	/** Enter/take critical lock. */
	#define CSP_ENTER_CRITICAL(lock) do { csp_bin_sem_wait(&lock, CSP_MAX_DELAY); } while(0)
	/** Exit/release critical lock. */
	#define CSP_EXIT_CRITICAL(lock) do { csp_bin_sem_post(&lock); } while(0)
#elif (CSP_FREERTOS)
	#include "FreeRTOS.h"
	#define CSP_BASE_TYPE portBASE_TYPE
	#define CSP_MAX_TIMEOUT portMAX_DELAY
	#define CSP_DEFINE_CRITICAL(lock)
	#define CSP_INIT_CRITICAL(lock) ({CSP_ERR_NONE;})
	#define CSP_ENTER_CRITICAL(lock) do { portENTER_CRITICAL(); } while (0)
	#define CSP_EXIT_CRITICAL(lock) do { portEXIT_CRITICAL(); } while (0)
#else
	#error "OS must be either CSP_POSIX, CSP_MACOSX, CSP_FREERTOS or CSP_WINDOWS"
#endif

/** Legacy definition for #CSP_MAX_TIMEOUT. */
#define CSP_MAX_DELAY CSP_MAX_TIMEOUT

/** Legacy definition for #CSP_MAX_TIMEOUT. */
#define CSP_INFINITY CSP_MAX_TIMEOUT

#ifdef __cplusplus
}
#endif
#endif
