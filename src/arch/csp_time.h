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

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <csp/csp.h>

/* Blackfin/x86 on Linux */
#if defined(CSP_POSIX)

#include <time.h>
#include <sys/time.h>
#include <limits.h>

#endif // CSP_POSIX

/* AVR/ARM on FreeRTOS */
#if defined(CSP_FREERTOS)

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#endif // CSP_FREERTOS

uint32_t csp_get_ms(void);
uint32_t csp_get_ms_isr(void);
uint32_t csp_get_s(void);
uint32_t csp_get_s_isr(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // _CSP_TIME_H_
