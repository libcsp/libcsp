

#include <csp/arch/csp_thread.h>

#include <time.h>
#include <errno.h>

void csp_sleep_ms(unsigned int time_ms) {

	struct timespec req, rem;
	req.tv_sec = (time_ms / 1000U);
	req.tv_nsec = ((time_ms % 1000U) * 1000000U);

	while ((nanosleep(&req, &rem) < 0) && (errno == EINTR)) {
		req = rem;
	}
}
