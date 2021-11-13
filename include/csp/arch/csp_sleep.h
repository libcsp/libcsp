#pragma once

/**
   @file

   Thread (task) interface.
*/

#include <csp/csp_types.h>

/**
   Sleep X mS.
   @param[in] time_ms mS to sleep.
*/
void csp_sleep_ms(unsigned int time_ms);
