/*
 * Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
 * Copyright (C) 2021 Space Cubics, LLC.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; If not, see <http://www.gnu.org/licenses/>.
 */

#include <csp/arch/csp_thread.h>

csp_thread_handle_t
csp_thread_create_static(csp_thread_handle_t * new_thread,
						 const char * const thread_name,
						 char * stack,
						 unsigned int stack_size,
						 csp_thread_func_t entry,
						 void * param,
						 unsigned int priority) {
	k_tid_t tid;

	tid = k_thread_create(new_thread,
						  stack, stack_size,
						  entry, param, NULL, NULL,
						  priority, 0, K_NO_WAIT);
	k_thread_name_set(tid, thread_name);

	return tid;
}

void csp_thread_exit(void) {
	k_thread_abort(k_current_get());
}

void csp_sleep_ms(unsigned int time_ms) {
	k_sleep(K_MSEC(time_ms));
}
