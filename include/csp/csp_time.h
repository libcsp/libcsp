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

#ifndef _CSP_TIME_H_
#define _CSP_TIME_H_

#include <stdint.h>
#include <csp/csp.h>

/* Blackfin/x86 on Linux */
#if defined(__CSP_POSIX__)

#include <time.h>
#include <sys/time.h>
#include <limits.h>

#define CSP_MAX_DELAY INT_MAX

#endif // __CSP_POSIX__

/* AVR/ARM on FreeRTOS */
#if defined(__CSP_FREERTOS__)

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define CSP_MAX_DELAY portMAX_DELAY

#endif // __CSP_FREERTOS__

uint32_t csp_get_ms();

#endif // _CSP_TIME_H_
