#include "csp_mutex.h"
#include <assert.h>
#include <stdlib.h>

void csp_mutex_init(csp_mutex_t * m) {
    pthread_mutexattr_t * attr = malloc(sizeof(pthread_mutexattr_t));
    assert(attr != NULL);
    assert(0 == pthread_mutexattr_init(attr));
    assert(0 == pthread_mutexattr_settype(attr, PTHREAD_MUTEX_RECURSIVE));
    assert(0 == pthread_mutexattr_setprotocol(attr, PTHREAD_PRIO_INHERIT));
    assert(0 == pthread_mutex_init(&m->handle, attr));
    assert(0 == pthread_mutexattr_destroy(attr));
    free(attr);
}

void csp_mutex_lock(csp_mutex_t * m) {
    assert(0 == pthread_mutex_lock(&m->handle));
}

void csp_mutex_unlock(csp_mutex_t * m) {
    assert(0 == pthread_mutex_unlock(&m->handle));
}
