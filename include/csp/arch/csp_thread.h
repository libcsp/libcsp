#pragma once

/**
   @file

   Thread (task) interface.
*/

#include <csp/csp_types.h>

#if (CSP_POSIX || CSP_MACOSX || __DOXYGEN__)
#include <pthread.h>

#elif (CSP_WINDOWS)
#include <windows.h>

#elif (CSP_FREERTOS)
#endif

/**
   Sleep X mS.
   @param[in] time_ms mS to sleep.
*/
void csp_sleep_ms(unsigned int time_ms);
