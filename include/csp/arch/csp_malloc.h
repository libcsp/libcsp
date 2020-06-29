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

#ifndef _CSP_MALLOC_H_
#define _CSP_MALLOC_H_

/**
   @file

   Memory interface.
*/

#include <csp/csp_platform.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
   Allocate chunk of memory (POSIX).
   @param[in] size size of memory chunk (bytes).
   @return Pointer to allocated memory, or NULL on failure.
*/
void * csp_malloc(size_t size);

/**
   Allocate chunk of memory and set it to zero (POSIX).
   @param[in] nmemb number of members/chunks to allocate.
   @param[in] size size of memory chunk (bytes).
   @return Pointer to allocated memory, or NULL on failure.
*/
void * csp_calloc(size_t nmemb, size_t size);

/**
   Free allocated memory (POSIX).
   @param[in] ptr memory to free. NULL pointer is ignored.
*/
void csp_free(void * ptr);

#ifdef __cplusplus
}
#endif
#endif
