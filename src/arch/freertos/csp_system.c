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

#include <stdio.h>
#include <stdint.h>
#include <FreeRTOS.h>
#include <task.h>

#include <csp/csp.h>
#include <csp/csp_error.h>

#include <csp/arch/csp_system.h>

extern void __attribute__((weak)) cpu_reset(void);
extern void __attribute__((weak)) cpu_shutdown(void);

int csp_sys_tasklist(char *out)
{
#if FREERTOS_VERSION < 8
	vTaskList((signed portCHAR *) out);
#else
	vTaskList(out);
#endif
	return CSP_ERR_NONE;
}

int csp_sys_tasklist_size(void)
{
	return 40 * uxTaskGetNumberOfTasks();
}

uint32_t csp_sys_memfree(void)
{
	uint32_t total = 0, max = UINT32_MAX, size;
	void *pmem;

	/* If size_t is less than 32 bits, start with 10 KiB */
	size = sizeof(uint32_t) > sizeof(size_t) ? 10000 : 1000000;

	while (1) {
		pmem = pvPortMalloc(size + total);
		if (pmem == NULL) {
			max = size + total;
			size = size / 2;
		} else {
			total += size;
			if (total + size >= max)
				size = size / 2;
			vPortFree(pmem);
		}
		if (size < 32) break;
	}

	return total;
}

int csp_sys_reboot(void)
{
	if (cpu_reset) {
		cpu_reset();
		while (1);
	}

	csp_log_error("Failed to reboot");

	return CSP_ERR_INVAL;
}

int csp_sys_shutdown(void)
{
	if (cpu_shutdown) {
		cpu_shutdown();
		while (1);
	}

	csp_log_error("Failed to shutdown");

	return CSP_ERR_INVAL;
}
