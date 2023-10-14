/****************************************************************************
 * **File:** csp/csp_buffer.h
 *
 * **Description:** Message buffer.
 ****************************************************************************/
#pragma once

#include <csp/csp_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get free buffer from task context.
 *
 * @param[in] unused OBSOLETE ignored field, csp packets have a fixed size now
 * @return Buffer pointer to #csp_packet_t or NULL if no buffers available
 */
csp_packet_t * csp_buffer_get(size_t unused);

/**
 * Get free buffer (from ISR context).
 *
 * @param[in] unused OBSOLETE ignored field, csp packets have a fixed size now
 * @return Buffer pointer to #csp_packet_t or NULL if no buffers available
 */
csp_packet_t * csp_buffer_get_isr(size_t unused);

/**
 * Free buffer (from task context).
 *
 * @param[in] buffer buffer to free. NULL is handled gracefully.
 */
void csp_buffer_free(void *buffer);

/**
 * Free buffer (from ISR context).
 *
 * @oaram[in] buffer buffer to free. NULL is handled gracefully.
 */
void csp_buffer_free_isr(void *buffer);

/**
 * Clone an existing buffer.
 * The existing \a buffer content is copied to the new buffer.
 *
 * @oaram[in] buffer buffer to clone.
 * @return cloned buffer on success, or NULL on failure.
 */
void * csp_buffer_clone(void *buffer);

/**
 * Return number of remaining/free buffers.
 * The number of buffers is set by csp_init().
 *
 * @return number of remaining/free buffers
 */
int csp_buffer_remaining(void);

void csp_buffer_init(void);

/**
 * Increase reference counter of buffer.
 * Use csp_buffer_free() to decrement
 *
 * @return CSP buffer data size.
 */
size_t csp_buffer_data_size(void);

void csp_buffer_init(void);

void csp_buffer_refc_inc(void * buffer);
  
#ifdef __cplusplus
}
#endif


