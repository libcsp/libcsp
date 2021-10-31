

#pragma once

/**
   @file

   Relative time interface.

   @note The returned values will eventually wrap.
*/

#include <csp/csp_types.h>



/**
   Return uptime in seconds.
   The function uses csp_get_s() for relative time. First time the function is called (by csp_init()), it saves an offset
   in case the platform doesn't start from 0, e.g. Linux.
   @return uptime in seconds.
*/
uint32_t csp_get_uptime_s(void);

/**
   Return current time in mS.
   @return mS.
*/
uint32_t csp_get_ms(void);

/**
   Return current time in mS (from ISR).
   @return mS.
*/
uint32_t csp_get_ms_isr(void);

/**
   Return current time in seconds.
   @return seconds.
*/
uint32_t csp_get_s(void);

/**
   Return current time in seconds (from ISR).
   @return seconds.
*/
uint32_t csp_get_s_isr(void);
