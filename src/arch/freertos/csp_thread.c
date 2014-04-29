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

#include <FreeRTOS.h>
#include <task.h>

/* CSP includes */
#include <csp/csp.h>

#include <csp/arch/csp_thread.h>

int csp_thread_create(csp_thread_return_t (* routine)(void *), const signed char * const thread_name, unsigned short stack_depth, void * parameters, unsigned int priority, csp_thread_handle_t * handle) {
#if (FREERTOS_VERSION >= 8)
	portBASE_TYPE ret = xTaskCreate(routine, (char *) thread_name, stack_depth, parameters, priority, handle);
#else
	portBASE_TYPE ret = xTaskCreate(routine, thread_name, stack_depth, parameters, priority, handle);
#endif
	if (ret != pdTRUE)
		return CSP_ERR_NOMEM;
	return CSP_ERR_NONE;
}
