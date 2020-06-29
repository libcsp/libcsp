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

#include <csp/arch/csp_system.h>

#include <csp/csp_debug.h>

static csp_sys_reboot_t csp_sys_reboot_callback = NULL;
static csp_sys_shutdown_t csp_sys_shutdown_callback = NULL;

void csp_sys_set_reboot(csp_sys_reboot_t reboot) {

	csp_sys_reboot_callback = reboot;

}

int csp_sys_reboot(void) {

	if (csp_sys_reboot_callback) {
		return (csp_sys_reboot_callback)();
	}
	csp_log_warn("%s not supported - no user function set", __FUNCTION__);
	return CSP_ERR_NOTSUP;

}

void csp_sys_set_shutdown(csp_sys_shutdown_t shutdown) {

	csp_sys_shutdown_callback = shutdown;

}

int csp_sys_shutdown(void) {

	if (csp_sys_shutdown_callback) {
		return (csp_sys_shutdown_callback)();
	}
	csp_log_warn("%s not supported - no user function set", __FUNCTION__);
	return CSP_ERR_NOTSUP;

}
