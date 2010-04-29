#include <pthread.h>

#include <time.h>
#include <sys/time.h>

#include <csp/csp.h>
#include <csp/time.h>

uint32_t csp_get_ms() {
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        return (ts.tv_sec*1000+ts.tv_nsec/1000000);
    } else {
        return 0;
    }
}
