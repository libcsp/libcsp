/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2010 Gomspace ApS (gomspace.com)
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

#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

/* CSP includes */
#include <csp/csp.h>
#include <csp/csp_config.h>

#if defined(_CSP_POSIX_)
#include <pthread.h>
#endif

#if CSP_DEBUG

static uint8_t levels_enable[7] = {
		0,	// Info
		1,	// Error
		1,	// Warn
		0,	// Buffer
		0,	// Packet
		0,	// Protocol
		0	// Locks
};

void csp_debug(csp_debug_level_t level, const char * format, ...) {

	const char * color = "";

	switch(level) {
	case CSP_INFO: 		if (!levels_enable[CSP_INFO]) return; 		color = ""; break;
	case CSP_ERROR: 	if (!levels_enable[CSP_ERROR]) return; 		color = "\E[1;31m"; break;
	case CSP_WARN: 		if (!levels_enable[CSP_WARN]) return; 		color = "\E[0;33m"; break;
	case CSP_BUFFER: 	if (!levels_enable[CSP_BUFFER]) return; 	color = "\E[0;33m"; break;
	case CSP_PACKET: 	if (!levels_enable[CSP_PACKET]) return; 	color = "\E[0;32m"; break;
	case CSP_PROTOCOL:  if (!levels_enable[CSP_PROTOCOL]) return; 	color = "\E[0;34m"; break;
	case CSP_LOCK:		if (!levels_enable[CSP_LOCK]) return; 		color = "\E[0;36m"; break;
	}
#if defined(_CSP_POSIX_)

	extern __attribute__((weak)) pthread_t handle_server;
	extern __attribute__((weak)) pthread_t handle_console;
	extern __attribute__((weak)) pthread_t handle_rdptestserver;
	extern __attribute__((weak)) pthread_t handle_router;

	if (pthread_self() == handle_server) {
		printf("\t\t\t\t\t\t\t\t");
	} else if (pthread_self() == handle_console) {
	} else if (pthread_self() == handle_rdptestserver) {
		printf("\t\t\t\t\t\t\t\t");
	} else if (pthread_self() == handle_router) {
#if defined(__i386__)
		printf("\t\t");
#else
		printf("\t\t\t\t");
#endif
	}
#endif

#if defined(_CSP_FREERTOS_)
#include <freertos/task.h>
#include "arch/csp_thread.h"

	extern csp_thread_handle_t handle_router;
	extern csp_thread_handle_t handle_server;
	extern csp_thread_handle_t handle_console;

	if (xTaskGetCurrentTaskHandle() == handle_server) {
		printf("\t\t\t\t\t\t\t\t");
	} else if (xTaskGetCurrentTaskHandle() == handle_console) {
	} else if (xTaskGetCurrentTaskHandle() == handle_router) {
		printf("\t\t\t\t");
	}
#endif

	printf("%s", color);

	va_list args;
    printf("CSP: ");
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    printf("\E[0m");


}

void csp_debug_toggle_level(csp_debug_level_t level) {
	if (level >= 7) {
		printf("Max level is 6\r\n");
		return;
	}
	levels_enable[level] = (levels_enable[level]) ? 0 : 1;
	printf("Level %u: value %u\r\n", level, levels_enable[level]);
}
#endif
