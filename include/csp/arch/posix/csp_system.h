

#pragma once

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
