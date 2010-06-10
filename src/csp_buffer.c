/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2010 GomSpace ApS (gomspace.com)
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

#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* CSP includes */
#include <csp/csp.h>
#include <csp/csp_config.h>

#include "arch/csp_malloc.h"
#include "arch/csp_semaphore.h"

#if CSP_BUFFER_STATIC
	typedef struct { uint8_t data[CSP_BUFFER_SIZE]; } csp_buffer_element_t;
	static csp_buffer_element_t csp_buffer[CSP_BUFFER_COUNT];
	static uint8_t csp_buffer_list[CSP_BUFFER_COUNT];
	static void * csp_buffer_p = &csp_buffer;
	static const int size = CSP_BUFFER_SIZE;
	static const int count = CSP_BUFFER_COUNT;
#else
	static uint8_t * csp_buffer_p;
	static uint8_t * csp_buffer_list;
	static int size, count;
#endif

int csp_buffer_init(int buf_count, int buf_size) {

#if CSP_BUFFER_STATIC == 0
	/* Remember size */
	count = buf_count;
	size = buf_size;

	/* Allocate main memory */
	csp_buffer_p = csp_malloc(count * size);
	if (csp_buffer_p == NULL)
		return 0;

	/* Allocate housekeeping memory */
	csp_buffer_list = (uint8_t *) csp_malloc(count * sizeof(uint8_t));
	if (csp_buffer_list == NULL) {
		csp_free(csp_buffer_p);
		return 0;
	}
#endif

	/* Clear housekeeping memory = all free mem */
	memset(csp_buffer_list, 0, count);

	return 1;

}

/**
 * Searched a statically assigned array for a free entry
 * Starts with the last given element + 1 for optimisation
 * This call is safe from both ISR and task context
 * @return poiter to a free csp_packet_t or NULL if out of memory
 */
void * csp_buffer_get(size_t buf_size) {

    static uint8_t csp_buffer_last_given = 0;

	if (buf_size + CSP_BUFFER_PACKET_OVERHEAD > size) {
		printf("Attempt to allocate too large block\r\n");
		return NULL;
	}

    CSP_ENTER_CRITICAL();
	int i = csp_buffer_last_given;							// Start with the last given element
	i = (i + 1) % count;									// Increment by one
	while(i != csp_buffer_last_given) {						// Loop till we have checked all
		if (csp_buffer_list[i] == CSP_BUFFER_FREE) {		// Check the buffer list
			csp_buffer_list[i] = CSP_BUFFER_USED;			// Mark as used
			csp_buffer_last_given = i;						// Remember the progress
			CSP_EXIT_CRITICAL();
#if CSP_BUFFER_CALLOC
			memset(csp_buffer_p + (i * size), 0x00, size);
#endif
			//printf("Found element %u, size %u, at %p\r\n", i, size, csp_buffer_p + (i * size));
			return csp_buffer_p + (i * size);				// Return poniter
		}
		i = (i + 1) % count;								// Increment by one
	}
	CSP_EXIT_CRITICAL();

	return NULL;											// If we are out of memory, return NULL
}

/**
 * Instantly free's the packet buffer
 * This call is safe from both ISR and Task context
 * @param packet
 */
void csp_buffer_free(void * packet) {
	int i = ((uint8_t *) packet - csp_buffer_p) / size;					// Find number in array by math (wooo)
	if (i < 0 || i > count)
		return;
	csp_buffer_list[i] = CSP_BUFFER_FREE;					// Mark this as free now
}

/**
 * Counts the amount of remaning buffers
 * @return Integer amount
 */
int csp_buffer_remaining(void) {
	int buf_count = 0, i;
	for(i = 0; i < count; i++) {
		if (csp_buffer_list[i] == CSP_BUFFER_FREE)
			buf_count++;
	}
	return buf_count;
}
