#include "csp_mutex.h"
#include <zephyr/sys/__assert.h>

void csp_mutex_init(csp_mutex_t * m) {
    __ASSERT_NO_MSG(0 == k_mutex_init(&m->handle));
}

void csp_mutex_lock(csp_mutex_t * m) {
    __ASSERT_NO_MSG(0 == k_mutex_lock(&m->handle, K_FOREVER));
}

void csp_mutex_unlock(csp_mutex_t * m) {
    __ASSERT_NO_MSG(0 == k_mutex_unlock(&m->handle));
}
