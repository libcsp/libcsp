#pragma once

#include "csp/autoconfig.h"

#if (CSP_ZEPHYR)
#include <zephyr/kernel.h>
#else
#define __noinit __attribute__((section(".noinit")))
#define __packed __attribute__((__packed__))
#define __maybe_unused __attribute__((__unused__))
#define __unused __attribute__((__unused__))

#define CONTAINER_OF(ptr, type, member) \
	((type *)(void *)((char *)(ptr) - offsetof(type, member)))

#endif

#if (CSP_WINDOWS)
// On Windows (PE/COFF format), .noinit sections get CONTENTS and LOAD flags. Since the .noinit optimization is useful
// for embedded systems, we don't use it for Windows builds and let these sections go to .bss (ALLOC only) section.
#undef __noinit
#define __noinit
#define __weak
#else
#define __weak __attribute__((__weak__))
#endif
