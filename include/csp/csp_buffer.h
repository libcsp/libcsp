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

#ifndef _CSP_BUFFER_H_
#define _CSP_BUFFER_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Start the buffer handling system
 * You must specify the number for buffers and the size. All buffers are fixed
 * size so you must specify the size of your largest buffer.
 *
 * @param count Number of buffers to allocate
 * @param size Buffer size in bytes.
 *
 * @return CSP_ERR_NONE if malloc() succeeded, CSP_ERR message otherwise.
 */
int csp_buffer_init(int count, int size);

/**
 * Get a reference to a free buffer. This function can only be called
 * from task context.
 *
 * @param size Specify what data-size you will put in the buffer
 * @return pointer to a free csp_packet_t or NULL if out of memory
 */
void * csp_buffer_get(size_t size);

/**
 * Get a reference to a free buffer. This function can only be called
 * from interrupt context.
 *
 * @param buf_size Specify what data-size you will put in the buffer
 * @return pointer to a free csp_packet_t or NULL if out of memory
 */
void * csp_buffer_get_isr(size_t buf_size);

/**
 * Free a buffer after use.
 * @param packet pointer to memory area, must be acquired by csp_buffer_get().
 */
void csp_buffer_free(void *packet);

/**
 * Free a buffer after use in ISR context.
 * @param packet pointer to memory area, must be acquired by csp_buffer_get().
 */
void csp_buffer_free_isr(void *packet);

/**
 * Clone an existing packet and increase/decrease cloned packet size.
 * @param buffer Existing buffer to clone.
 */
void * csp_buffer_clone(void *buffer);

/**
 * Return how many buffers that are currently free.
 * @return number of free buffers
 */
int csp_buffer_remaining(void);

/**
 * Return the size of the CSP buffers
 * @return size of CSP buffers
 */
int csp_buffer_size(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _CSP_BUFFER_H_ */
