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

#ifndef _CSP_ARCH_POSIX_CSP_SYSTEM_H_
#define _CSP_ARCH_POSIX_CSP_SYSTEM_H_

/**
   @file

   Posix extension to system interface.
*/

#include <csp/arch/csp_system.h>

#ifdef __cplusplus
yextern "C" {
#endif

/**
   Executes 'system("reboot")' for system reboot.
*/
int csp_sys_reboot_using_system(void);

/**
   Executes 'sync() and reboot(LINUX_REBOOT_CMD_RESTART)' for system reboot.
*/
int csp_sys_reboot_using_reboot(void);

/**
   Executes 'system("halt")' for system shutdown.
*/
int csp_sys_shutdown_using_system(void);

/**
   Executes 'sync() and reboot(LINUX_REBOOT_CMD_HALT)' for system shutdown.
*/
int csp_sys_shutdown_using_reboot(void);

#ifdef __cplusplus
}
#endif
#endif
