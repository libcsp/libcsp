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

#include <stdint.h>
#include <pthread.h>
#include <limits.h>

/* CSP includes */
#include <csp/csp.h>

#include <csp/arch/csp_thread.h>

int csp_thread_create(csp_thread_return_t (* routine)(void *), const char * const thread_name, unsigned short stack_depth, void * parameters, unsigned int priority, csp_thread_handle_t * handle) {
	pthread_attr_t attributes, *attr_ref;
	int return_code;
	
	if( pthread_attr_init(&attributes) == 0 )
	{
		unsigned int stack_size = PTHREAD_STACK_MIN;// use at least one memory page
		
		while(stack_size < stack_depth)// must reach at least the provided size
		{
			stack_size += PTHREAD_STACK_MIN;// keep memory page boundary (some systems may fail otherwise))
		}
		attr_ref = &attributes;
		
		pthread_attr_setdetachstate(attr_ref, PTHREAD_CREATE_DETACHED);// do not waste memory on each call
		pthread_attr_setstacksize(attr_ref, stack_size);
	}
	else
	{
		attr_ref = NULL;
	}
	return_code = pthread_create(handle, attr_ref, routine, parameters);
	if( attr_ref != NULL ) pthread_attr_destroy(attr_ref);
	
	return return_code;
}
