/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2011 GomSpace ApS (http://www.gomspace.com)
Copyright (C) 2011 AAUSAT3 Project (http://aausat3.space.aau.dk) 

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

typedef enum csp_buffer_state_t {
	CSP_BUFFER_FREE	= 0,
	CSP_BUFFER_USED	= 1,
} csp_buffer_state_t;

#if CSP_BUFFER_STATIC
	typedef struct { uint8_t data[CSP_BUFFER_SIZE]; } csp_buffer_element_t;
	static csp_buffer_element_t csp_buffer[CSP_BUFFER_COUNT];
	static csp_buffer_state csp_buffer_list[CSP_BUFFER_COUNT];
	static void * csp_buffer_p = &csp_buffer;
	static const size_t = CSP_BUFFER_SIZE;
	static const int count = CSP_BUFFER_COUNT;
#else
	static uint8_t * csp_buffer_p;
	static csp_buffer_state_t * csp_buffer_list;
	static size_t size;
	static int count;
#endif

#ifdef _CSP_POSIX_
static csp_bin_sem_handle_t csp_critical_lock;
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
	csp_buffer_list = (csp_buffer_state_t *) csp_malloc(count * sizeof(csp_buffer_state_t));
	if (csp_buffer_list == NULL) {
		csp_free(csp_buffer_p);
		return 0;
	}
#endif

#ifdef _CSP_POSIX_
	/* Initialize critical lock */
	if (csp_bin_sem_create(&csp_critical_lock) != CSP_SEMAPHORE_OK) {
		csp_debug(CSP_ERROR, "No more memory for buffer semaphore\r\n");

		if (csp_buffer_list)
			csp_free(csp_buffer_list);
		if (csp_buffer_p)
			csp_free(csp_buffer_p);

		return 0;
	}
#endif

	/* Clear housekeeping memory = all free mem */
	memset(csp_buffer_list, 0, count * sizeof(csp_buffer_state_t));

	return 1;

}

void * csp_buffer_get_isr(size_t buf_size) {

	static uint8_t csp_buffer_last_given = 0;

	if (buf_size + CSP_BUFFER_PACKET_OVERHEAD > size) {
		csp_debug(CSP_ERROR, "Attempt to allocate too large block %u\r\n", buf_size);
		return NULL;
	}

	int i = csp_buffer_last_given; // Start with the last given element
	i = (i + 1) % count; // Increment by one
	while (i != csp_buffer_last_given) { // Loop till we have checked all
		if (csp_buffer_list[i] == CSP_BUFFER_FREE) { // Check the buffer list
			csp_buffer_list[i] = CSP_BUFFER_USED; // Mark as used
			csp_buffer_last_given = i; // Remember the progress
#if CSP_BUFFER_CALLOC
			memset(csp_buffer_p + (i * size), 0x00, size);
#endif
			csp_debug(CSP_BUFFER, "BUFFER: Using element %u at %p\r\n", i, csp_buffer_p + (i * size));
			return csp_buffer_p + (i * size); // Return poniter
		}
		i = (i + 1) % count; // Increment by one
	}

	csp_debug(CSP_ERROR, "Out of buffers\r\n");
	return NULL; // If we are out of memory, return NULL

}

/**
 * Searched a statically assigned array for a free entry
 * Starts with the last given element + 1 for optimisation
 * This call is safe from both ISR and task context
 * @return poiter to a free csp_packet_t or NULL if out of memory
 */
void * csp_buffer_get(size_t buf_size) {
	void * buffer;
	CSP_ENTER_CRITICAL(csp_critical_lock);
	buffer = csp_buffer_get_isr(buf_size);
	CSP_EXIT_CRITICAL(csp_critical_lock);
	return buffer;
}

/**
 * Instantly free's the packet buffer
 * This call is safe from both ISR and Task context
 * @param packet
 */
void csp_buffer_free(void * packet) {
	int i = ((uint8_t *) packet - csp_buffer_p) / size;					// Find number in array by math (wooo)
	csp_debug(CSP_BUFFER, "BUFFER: Free element %u\r\n", i);
	if (i < 0 || i > count)
		return;
	csp_buffer_list[i] = CSP_BUFFER_FREE;					// Mark this as free now
}

/**
 * Clone an existing packet.
 * @param buffer Existing buffer to clone.
 */
void * csp_buffer_clone(void * buffer) {

	csp_packet_t * packet = (csp_packet_t *)buffer;

	if (!packet)
		return NULL;

	csp_packet_t * clone = csp_buffer_get(packet->length);

	if (clone)
		memcpy(&clone->length, &packet->length, packet->length + sizeof(csp_id_t) + sizeof(uint16_t));

	return clone;

}

int csp_buffer_remaining(void) {
	int buf_count = 0, i;
	for(i = 0; i < count; i++) {
		if (csp_buffer_list[i] == CSP_BUFFER_FREE)
			buf_count++;
	}
	return buf_count;
}

#if CSP_DEBUG
void csp_buffer_print_table(void) {
	int i;
	csp_packet_t * packet;
	for(i = 0; i < count; i++) {
		printf("[%02u] ", i);
		printf("%s ", csp_buffer_list[i] == CSP_BUFFER_FREE ? "FREE" : "USED");
		packet = (csp_packet_t *) (csp_buffer_p + (i * size));
		printf("Packet P 0x%02X, S 0x%02X, D 0x%02X, Dp 0x%02X, Sp 0x%02X",
			packet->id.pri, packet->id.src, packet->id.dst, packet->id.dport,
			packet->id.sport);
		printf("\r\n");
	}
}
#endif
