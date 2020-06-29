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

#ifndef _CSP_TIME_H_
#define _CSP_TIME_H_

/**
   @file

   Relative time interface.

   @note The returned values will eventually wrap.
*/

#include <csp/csp_platform.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
   Return uptime in seconds.
   The function uses csp_get_s() for relative time. First time the function is called (by csp_init()), it saves an offset
   in case the platform doesn't start from 0, e.g. Linux.
   @return uptime in seconds.
*/
uint32_t csp_get_uptime_s(void);

/**
   Return current time in mS.
   @return mS.
*/
uint32_t csp_get_ms(void);

/**
   Return current time in mS (from ISR).
   @return mS.
*/
uint32_t csp_get_ms_isr(void);

/**
   Return current time in seconds.
   @return seconds.
*/
uint32_t csp_get_s(void);

/**
   Return current time in seconds (from ISR).
   @return seconds.
*/
uint32_t csp_get_s_isr(void);

#ifdef __cplusplus
}
#endif
#endif
