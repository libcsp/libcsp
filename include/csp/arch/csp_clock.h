

#pragma once

/**
   @file

   Clock interface.
*/

#include <csp/csp_types.h>



/**
   Timestamp (cross platform).
*/
typedef struct {
        //! Seconds
	uint32_t tv_sec;
        //! Nano-seconds.
	uint32_t tv_nsec;
} csp_timestamp_t;

/**
   Get time.

   This function is 'weak' in libcsp, providing a working implementation for following OS's: POSIX, Windows and Macosx.
   This function is expected to be equivalent to standard POSIX clock_gettime(CLOCK_REALTIME, ...).

   @param[out] time current time.
*/
void csp_clock_get_time(csp_timestamp_t * time);

/**
   Set time.

   This function is 'weak' in libcsp, providing a working implementation for following OS's: POSIX, Windows and Macosx.
   This function is expected to be equivalent to standard POSIX clock_settime(CLOCK_REALTIME, ...).

   @param[in] time time to set.
   @return #CSP_ERR_NONE on success.
*/
int csp_clock_set_time(const csp_timestamp_t * time);

/* Additional functions for 64-bit nanosecond type */
uint64_t clock_get_time64(void);
void clock_set_time64(uint64_t time);

