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

/**
 * @file csp_buffer.h
 * @brief Global buffer handler used by low-level network interfaces.
 *
 * GomSpace uses a fixed-buffer system for all its network interfaces. This
 * means that buffers are allocated at system startup instead of when a message
 * is incoming. As a consequence, a buffer must be allocated with the
 * csp_buffer_get and free'd with the csp_buffer_free commands.
 *
 * The advantage is that all buffer functions are thread-safe and
 * interrupt-safe. Also incoming interrupt-service routines can allocate memory
 * much quicket than with the traditional malloc calls.
 *
 * @addtogroup BufferHandling
 * @{
 *
 */

#ifndef CSP_BUFFER_H_
#define CSP_BUFFER_H_

/**
 * Start the buffer handling system
 * You must specify the number for buffers and the size. All buffers are fixed
 * size so you must specify the size of your largest buffer. The standard for
 * GomSpace network library is 310 bytes.
 *
 * @param count: Number of buffers to allocate
 * @param size: Buffer size in bytes.
 *
 * @return 0 if malloc() failed, 1 if sucessful.
 */
int csp_buffer_init(int count, int size);

/**
 * Get a reference to a free buffer. This call is both interrupt and thread
 * safe. This function is similar to malloc()
 *
 * @param size: Specify what data-size you will put in the buffer
 * @return NULL if size is larger than buffer-block-sizse, pointer otherwise.
 */
void * csp_buffer_get(size_t size);

/**
 * Free a buffer after use. This call is both interrupt and thread safe.
 * This function is similar to free()
 *
 * @param pointer to memory area, must be aquired by csp_buffer_get().
 */
void csp_buffer_free(void * packet);

/**
 * Return how many buffers that are currently free.
 * @return number of free buffers
 */
int csp_buffer_remaining(void);

#endif /* CSP_BUFFER_H_ */

/** }@ */
