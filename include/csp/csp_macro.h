#pragma once

#include "csp/autoconfig.h"

#if (CSP_ZEPHYR)
#include <zephyr/kernel.h>
#else
#define __noinit __attribute__((section(".noinit")))
#endif
