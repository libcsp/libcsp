/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2012 Gomspace ApS (http://www.gomspace.com)
Copyright (C) 2012 AAUSAT3 Project (http://aausat3.space.aau.dk) 

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <csp/arch/csp_clock.h>

#include <windows.h>

#include <csp/csp_debug.h>

//__attribute__((weak)) void csp_clock_get_time(csp_timestamp_t * time) {
void csp_clock_get_time(csp_timestamp_t * time) {

    FILETIME ftime; // 64-bit, representing the number of 100-nanosecond intervals since January 1, 1601 (UTC).
    GetSystemTimeAsFileTime(&ftime);
    ULARGE_INTEGER itime = {.LowPart = ftime.dwLowDateTime, .HighPart = ftime.dwHighDateTime};
    itime.QuadPart -= 116444736000000000ULL;  // 1 jan 1601 to 1 jan 1970
    time->tv_sec = itime.QuadPart / (1000ULL * 1000ULL * 10ULL);
    time->tv_nsec = (itime.QuadPart % (1000ULL * 1000ULL * 10ULL)) * 100ULL;
}

//__attribute__((weak)) int csp_clock_set_time(const csp_timestamp_t * time) {
int csp_clock_set_time(const csp_timestamp_t * time) {
    csp_log_warn("csp_clock_set_time() not supported");
    return CSP_ERR_NOTSUP;
}
