#pragma once

#include "csp/autoconfig.h"

#if (CSP_ZEPHYR)
#include <zephyr/kernel.h>
#else
#define __noinit __attribute__((section(".noinit")))
#define __packed __attribute__((__packed__))
#define __unused __attribute__((__unused__))
#define __weak   __attribute__((__weak__))
#endif
