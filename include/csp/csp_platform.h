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

#ifndef _CSP_PLATFORM_H_
#define _CSP_PLATFORM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* Attempt to include endian.h to get endianness defines */
#ifdef CSP_HAVE_ENDIAN_H
	#include <endian.h>
#endif

/* Set platform endianness */
#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && defined(__ORDER_BIG_ENDIAN__)
	#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
		#define CSP_LITTLE_ENDIAN
	#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
		#define CSP_BIG_ENDIAN
	#else
		/* We don't support PDP endianness */
		#error "Unsupported endianness"
	#endif
#else
	/* Try to guess system endianness */
	#if defined(__i386__) || defined(__x86_64__) || defined(__BFIN__) || defined(__AVR__) || defined(__ARMEL__)
		#define CSP_LITTLE_ENDIAN
	#elif defined (__PPC__) || defined(__sparc__) || defined(__AVR32__) || defined(__AVR32_AP7000__) || defined(__ARMEB__)
		#define CSP_BIG_ENDIAN
	#else
		#error "Could not guess system endianness"
	#endif
#endif

/* Set OS */
#if defined(CSP_POSIX) || defined(CSP_WINDOWS)
	#define CSP_BASE_TYPE int
	#define CSP_MAX_DELAY (UINT32_MAX)
	#define CSP_INFINITY (UINT32_MAX)
	#define CSP_ENTER_CRITICAL(lock) do { csp_bin_sem_wait(&lock, CSP_MAX_DELAY); } while(0)
	#define CSP_EXIT_CRITICAL(lock) do { csp_bin_sem_post(&lock); } while(0)
#elif defined(CSP_FREERTOS)
	#include <freertos/FreeRTOS.h>
	#define CSP_BASE_TYPE portBASE_TYPE
	#define CSP_MAX_DELAY portMAX_DELAY
	#define CSP_INFINITY portMAX_DELAY
	#define CSP_ENTER_CRITICAL(lock) do { portENTER_CRITICAL(); } while (0)
	#define CSP_EXIT_CRITICAL(lock) do { portEXIT_CRITICAL(); } while (0)
#else
	#error "OS must be either CSP_POSIX, CSP_FREERTOS OR CSP_WINDOWS"
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // _CSP_PLATFORM_H_
