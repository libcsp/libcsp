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

#ifndef _CSP_MALLOC_H_
#define _CSP_MALLOC_H_

#include <stdint.h>

/* POSIX interface */
#if defined(__CSP_POSIX__)

#include <stdlib.h>

#endif // __CSP_POSIX__

/* FreeRTOS interface */
#if defined(__CSP_FREERTOS__)

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#endif // __CSP_FREERTOS__

void * csp_malloc(size_t size);
void csp_free(void * ptr);

#endif // _CSP_MALLOC_H_
