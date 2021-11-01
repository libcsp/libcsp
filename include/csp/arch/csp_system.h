

#pragma once

/**
   @file

   System interface.
*/

#include <csp/csp_types.h>



/** Color mask */
#define COLOR_MASK_COLOR 	0x0F
/** Color modifier mask */
#define COLOR_MASK_MODIFIER	0xF0

/**
   Color and color modifiers.
*/
typedef enum {
	/* Colors */
	COLOR_RESET		= 0xF0,
	COLOR_BLACK		= 0x01,
	COLOR_RED		= 0x02,
	COLOR_GREEN		= 0x03,
	COLOR_YELLOW		= 0x04,
	COLOR_BLUE		= 0x05,
	COLOR_MAGENTA		= 0x06,
	COLOR_CYAN		= 0x07,
	COLOR_WHITE		= 0x08,
	/* Modifiers */
	COLOR_NORMAL		= 0x0F,
	COLOR_BOLD		= 0x10,
	COLOR_UNDERLINE		= 0x20,
	COLOR_BLINK		= 0x30,
	COLOR_HIDE		= 0x40,
} csp_color_t;

/**
   Get task list.
   Write task list into a pre-allocate buffer. The buffer must be at least the size returned by csp_sys_tasklist_size().
   @param[out] out pre-allocate buffer for returning task.
   @return #CSP_ERR_NONE on success.
*/
int csp_sys_tasklist(char * out);

/**
   Get size of task buffer.
   @return Size of task list buffer to allocate for the csp_sys_tasklist().
*/
int csp_sys_tasklist_size(void);

/**
   Free system memory.

   @return Free system memory (bytes)
*/
uint32_t csp_sys_memfree(void);

/**
   Callback function for system reboot request.
   @return #CSP_ERR_NONE on success (if function returns at all), or error code.
*/
typedef int (*csp_sys_reboot_t)(void);

/**
   Set system reboot/reset function.
   Function will be called by csp_sys_reboot().
   @param[in] reboot callback.
   @see csp_sys_reboot_using_system(), csp_sys_reboot_using_reboot()
*/
void csp_sys_set_reboot(csp_sys_reboot_t reboot);

/**
   Reboot/reset system.
   Reboot/resets the system by calling the function set by csp_sys_set_reboot().
   @return #CSP_ERR_NONE on success (if function returns at all), or error code.
*/
int csp_sys_reboot(void);

/**
   Callback function for system shutdown request.
   @return #CSP_ERR_NONE on success (if function returns at all), or error code.
*/
typedef int (*csp_sys_shutdown_t)(void);

/**
   Set system shutdown function.
   Function will be called by csp_sys_shutdown().
   @param[in] shutdown callback.
   @see csp_sys_shutdown_using_system(), csp_sys_shutdown_using_reboot()
*/
void csp_sys_set_shutdown(csp_sys_shutdown_t shutdown);

/**
   Shutdown system.
   Shuts down the system by calling the function set by csp_sys_set_shutdown().
   @return #CSP_ERR_NONE on success (if function returns at all), or error code.
*/
int csp_sys_shutdown(void);

/**
   Set color on stdout.
   @param[in] color color.
*/
void csp_sys_set_color(csp_color_t color);
