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

#ifndef _CSP_ARCH_CLOCK_H_
#define _CSP_ARCH_CLOCK_H_

/**
   @file

   Clock interface.
*/

#include <csp/csp_platform.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
   Timestamp (cross platform).
*/
typedef struct {
        //! Seconds
	uint32_t tv_sec;
        //! Nano-seconds.
	uint32_t tv_nsec;
} csp_timestamp_t;

/**
   Get time.

   This function is 'weak' in libcsp, providing a working implementation for following OS's: POSIX, Windows and Macosx.
   This function is expected to be equivalent to standard POSIX clock_gettime(CLOCK_REALTIME, ...).

   @param[out] time current time.
*/
void csp_clock_get_time(csp_timestamp_t * time);

/**
   Set time.

   This function is 'weak' in libcsp, providing a working implementation for following OS's: POSIX, Windows and Macosx.
   This function is expected to be equivalent to standard POSIX clock_settime(CLOCK_REALTIME, ...).

   @param[in] time time to set.
   @return #CSP_ERR_NONE on success.
*/
int csp_clock_set_time(const csp_timestamp_t * time);

#ifdef __cplusplus
}
#endif
#endif
