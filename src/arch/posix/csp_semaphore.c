#include <semaphore.h>
#include <pthread.h>

#include <csp/csp.h>
#include <csp/semaphore.h>

int csp_bin_sem_create(csp_bin_sem_handle_t * sem) {
    if (sem_init(sem, 0, 1) == 0) {
        return CSP_SEMAPHORE_OK;
    } else {
        return CSP_SEMAPHORE_ERROR;
    }
}

int csp_bin_sem_wait(csp_bin_sem_handle_t * sem, int timeout) {
    struct timeval tv;
    struct timespec ts;
    if (gettimeofday(&tv, NULL))
    	return CSP_SEMAPHORE_ERROR;
    ts.tv_sec  = tv.tv_sec;
    ts.tv_nsec = tv.tv_usec * 1000;
    ts.tv_sec += timeout;

    if (sem_timedwait(sem, &ts) == 0) {
        return CSP_SEMAPHORE_OK;
    } else {
        return CSP_SEMAPHORE_ERROR;
    }
}

int csp_bin_sem_post(csp_bin_sem_handle_t * sem) {
    signed char dummy = 0;
    return csp_bin_sem_post_isr(sem, &dummy);
}

int csp_bin_sem_post_isr(csp_bin_sem_handle_t * sem, signed char * task_woken) {
    *task_woken = 0; 
    if (sem_post(sem) == 0) {
        return CSP_SEMAPHORE_OK;
    } else {
        return CSP_SEMAPHORE_ERROR;
    }
}
