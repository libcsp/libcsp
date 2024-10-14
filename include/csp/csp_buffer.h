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
 * Get a buffer or get killed (from task context)
 * @return Buffer (pointer to #csp_packet_t)
 */
csp_packet_t * csp_buffer_get_always(void);

/**
 * Get free buffer (from ISR context).
 *
 * @param[in] unused OBSOLETE ignored field, csp packets have a fixed size now
 * @return Buffer pointer to #csp_packet_t or NULL if no buffers available
 */
csp_packet_t * csp_buffer_get_isr(size_t unused);

/**
 * Get a buffer or get killed (from ISR context)
 * @return Buffer (pointer to #csp_packet_t)
 */
csp_packet_t * csp_buffer_get_always_isr(void);

/**
 * Free buffer (from task context).
 *
 * @param[in] buffer buffer to free. NULL is handled gracefully.
 */
void csp_buffer_free(void * buffer);

/**
 * Free buffer (from ISR context).
 *
 * @param[in] buffer buffer to free. NULL is handled gracefully.
 */
void csp_buffer_free_isr(void * buffer);

/**
 * Clone an existing buffer.
 * The existing \a buffer content is copied to the new buffer.
 *
 * @param[in] buffer buffer to clone.
 * @return cloned buffer on success, or NULL on failure.
 */
csp_packet_t * csp_buffer_clone(const csp_packet_t * packet);

/**
 * Copy the contents of a buffer.
 *
 * @param[in] src Source buffer.
 * @param[out] dst Destination buffer.
 */
void csp_buffer_copy(const csp_packet_t * src, csp_packet_t * dst);

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
 * @param[in] buffer buffer to increment. NULL is handled gracefully.
 */
void csp_buffer_refc_inc(void * buffer);

#ifdef __cplusplus
}
#endif
