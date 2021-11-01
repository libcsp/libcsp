

#pragma once

/**
   @file
   Message buffer.
*/

#include <csp/csp_types.h>

/**
   Get free buffer (from task context).

   @param[in] data_size minimum data size of requested buffer.
   @return Buffer (pointer to #csp_packet_t) or NULL if no buffers available or size too big.
*/
void * csp_buffer_get(size_t data_size);

/**
   Get free buffer (from ISR context).

   @param[in] data_size minimum data size of requested buffer.
   @return Buffer (pointer to #csp_packet_t) or NULL if no buffers available or size too big.
*/
void * csp_buffer_get_isr(size_t data_size);

/**
   Free buffer (from task context).
   @param[in] buffer buffer to free. NULL is handled gracefully.
*/
void csp_buffer_free(void *buffer);

/**
   Free buffer (from ISR context).
   @param[in] buffer buffer to free. NULL is handled gracefully.
*/
void csp_buffer_free_isr(void *buffer);

/**
   Clone an existing buffer.
   The existing \a buffer content is copied to the new buffer.
   @param[in] buffer buffer to clone.
   @return cloned buffer on success, or NULL on failure.
*/
void * csp_buffer_clone(void *buffer);

/**
   Return number of remaining/free buffers.
   The number of buffers is set by csp_init().
   @return number of remaining/free buffers
*/
int csp_buffer_remaining(void);

/**
   Return the size of a CSP buffer.
   @return size of a CSP buffer, sizeof(#csp_packet_t) + data_size.
*/
size_t csp_buffer_size(void);

/**
   Return the data size of a CSP buffer.
   The data size is set by csp_init().
   @return data size of a CSP buffer
*/
size_t csp_buffer_data_size(void);

void csp_buffer_init(void);


