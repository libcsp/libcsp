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

#ifndef _CSP_PLATFORM_H_
#define _CSP_PLATFORM_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Set platform endianness and OS */
#if defined(__i386__) || defined(__x86_64__) || defined(__BFIN__)
    #define _CSP_LITTLE_ENDIAN_
    #define _CSP_POSIX_
#elif defined(__AVR__) || defined(__arm__)
    #define _CSP_LITTLE_ENDIAN_
    #define _CSP_FREERTOS_
#elif defined (__PPC__) || defined(__sparc__)
    #define _CSP_BIG_ENDIAN_
    #define _CSP_POSIX_    
#elif defined(__AVR32__) || defined(__AVR32_AP7000__)
    #define _CSP_BIG_ENDIAN_
    #define _CSP_FREERTOS_
#else
    #error "Unknown architecture"
#endif

/* Set OS dependant features */
#if defined(_CSP_POSIX_)
    #define CSP_BASE_TYPE int
    #define CSP_MAX_DELAY (1000000000)
	#define CSP_ENTER_CRITICAL() do { csp_bin_sem_wait(&csp_global_lock, CSP_MAX_DELAY); } while(0)
	#define CSP_EXIT_CRITICAL() do { csp_bin_sem_post(&csp_global_lock); } while(0)
#elif defined(_CSP_FREERTOS_)
    #include <freertos/FreeRTOS.h>
    #define CSP_BASE_TYPE portBASE_TYPE
    #define CSP_MAX_DELAY portMAX_DELAY
	#define CSP_ENTER_CRITICAL() portENTER_CRITICAL()
	#define CSP_EXIT_CRITICAL() portEXIT_CRITICAL()
#else
    #error "Unknown architecture"
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // _CSP_PLATFORM_H_
