

#include <csp/arch/csp_clock.h>

#include <windows.h>

#include <csp/csp_debug.h>

//__weak void csp_clock_get_time(csp_timestamp_t * time) {
void csp_clock_get_time(csp_timestamp_t * time) {

	FILETIME ftime;  // 64-bit, representing the number of 100-nanosecond intervals since January 1, 1601 (UTC).
	GetSystemTimeAsFileTime(&ftime);
	ULARGE_INTEGER itime = {.LowPart = ftime.dwLowDateTime, .HighPart = ftime.dwHighDateTime};
	itime.QuadPart -= 116444736000000000ULL;  // 1 jan 1601 to 1 jan 1970
	time->tv_sec = itime.QuadPart / (1000ULL * 1000ULL * 10ULL);
	time->tv_nsec = (itime.QuadPart % (1000ULL * 1000ULL * 10ULL)) * 100ULL;
}

//__weak int csp_clock_set_time(const csp_timestamp_t * time) {
int csp_clock_set_time(const csp_timestamp_t * time) {
	return CSP_ERR_NOTSUP;
}
