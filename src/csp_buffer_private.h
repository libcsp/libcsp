#pragma once

#include <csp/csp_types.h>

/**
 * Get a buffer or get killed (from task context)
 *
 * This function return a buffer or kill the whole program when it
 * failed. DO NOT USE THIS FUNCTION if you don't know what you are
 * doing.  Never use this function from application layer.  It is
 * intended for internal use only and must not be used from
 * application-level code.
 *
 * https://github.com/libcsp/libcsp/issues/864
 *
 * @return Buffer (pointer to #csp_packet_t)
 */
csp_packet_t * csp_buffer_get_always(void);

/**
 * Get a buffer or get killed (from ISR context)
 * @return Buffer (pointer to #csp_packet_t)
 */
csp_packet_t * csp_buffer_get_always_isr(void);
