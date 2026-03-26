#include <windows.h>
#include <csp/arch/csp_time.h>

#include <time.h>
#include <sys/time.h>
#include <limits.h>

#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME 0
#endif

#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 1
#endif

/*
 * Emulated clock_gettime function for Windows.
 *
 * clock_id  - clock to use: CLOCK_REALTIME or CLOCK_MONOTONIC
 * tp        - pointer to timespec structure to fill with the current time
 *
 * returns 0 on success, or -1 on failure.
 */
int clock_gettime(int clock_id, struct timespec* tp) {
	if (!tp)
		return -1;

	switch(clock_id) {
	case CLOCK_MONOTONIC: {
		/* Use QueryPerformanceCounter for CLOCK_MONOTONIC */
		static LARGE_INTEGER frequency = {0};
		if (!frequency.QuadPart) {
			if (!QueryPerformanceFrequency(&frequency))
				return -1;  // Failure to get frequency
		}

		LARGE_INTEGER counter;
		if (!QueryPerformanceCounter(&counter))
			return -1;  // Failure to get counter

		tp->tv_sec = (time_t)(counter.QuadPart / frequency.QuadPart);
		tp->tv_nsec = (long)(((counter.QuadPart % frequency.QuadPart) * 1000000000LL) / frequency.QuadPart);
		return 0;
	}
	case CLOCK_REALTIME: {
		/* Use GetSystemTimeAsFileTime for CLOCK_REALTIME */
		FILETIME ft;
		GetSystemTimeAsFileTime(&ft);

		ULARGE_INTEGER uli;
		uli.LowPart = ft.dwLowDateTime;
		uli.HighPart = ft.dwHighDateTime;

		const ULONGLONG EPOCH_DIFFERENCE = 116444736000000000ULL;  // in 100-ns intervals
		ULONGLONG time100ns = uli.QuadPart - EPOCH_DIFFERENCE;
		tp->tv_sec = (time_t)(time100ns / 10000000ULL);
		tp->tv_nsec = (long)((time100ns % 10000000ULL) * 100);
		return 0;
	}
	}

	return -1;
}

uint32_t csp_get_ms(void) {

	struct timespec ts;
	if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
		return (uint32_t)((ts.tv_sec * 1000) + (ts.tv_nsec / 1000000));
	}
	return 0;
}

uint32_t csp_get_ms_isr(void) {

	return csp_get_ms();
}

uint32_t csp_get_s(void) {

	struct timespec ts;
	if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
		return (uint32_t)ts.tv_sec;
	}
	return 0;
}

uint32_t csp_get_s_isr(void) {

	return csp_get_s();
}
