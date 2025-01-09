#pragma once

#include "csp/autoconfig.h"

#if (CSP_ZEPHYR)
#include <zephyr/kernel.h>
#else
#define __noinit __attribute__((section(".noinit")))
#define __packed __attribute__((__packed__))
#define __maybe_unused __attribute__((__unused__))
#define __unused __attribute__((__unused__))
#ifdef __CYGWIN__
#define __weak
#else
#define __weak   __attribute__((__weak__))
#endif

#define CONTAINER_OF(ptr, type, member) \
	((type *)(void *)((char *)(ptr) - offsetof(type, member)))

#endif
