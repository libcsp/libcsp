#pragma once

#include "csp/autoconfig.h"

#if CSP_WINDOWS
int rand_r(unsigned int * seedp);
#elif CSP_POSIX || CSP_FREERTOS || CSP_ZEPHYR
#include <stdlib.h>
#else
#error Platform does not support rand_r() function
#endif
