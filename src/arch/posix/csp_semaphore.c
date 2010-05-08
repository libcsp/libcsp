/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2010 Gomspace ApS (gomspace.com)
Copyright (C) 2010 AAUSAT3 Project (aausat3.space.aau.dk) 

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

#include <semaphore.h>
#include <pthread.h>
#include <sys/time.h>

/* CSP includes */
#include <csp/csp.h>

#include "../csp_semaphore.h"

int csp_bin_sem_create(csp_bin_sem_handle_t * sem) {
    if (sem_init(sem, 0, 1) == 0) {
        return CSP_SEMAPHORE_OK;
    } else {
        return CSP_SEMAPHORE_ERROR;
    }
}

int csp_bin_sem_wait(csp_bin_sem_handle_t * sem, int timeout) {
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts))
    	return CSP_SEMAPHORE_ERROR;
    
    uint32_t sec = timeout / 1000;
    uint32_t nsec = (timeout - 1000 * sec) * 1000000;

    ts.tv_sec += sec;
    ts.tv_nsec += nsec;

    if (ts.tv_nsec + nsec > 1000000000)
        ts.tv_sec++;

    if (sem_timedwait(sem, &ts) == 0) {
        return CSP_SEMAPHORE_OK;
    } else {
        return CSP_SEMAPHORE_ERROR;
    }
}

int csp_bin_sem_post(csp_bin_sem_handle_t * sem) {
    CSP_BASE_TYPE dummy = 0;
    return csp_bin_sem_post_isr(sem, &dummy);
}

int csp_bin_sem_post_isr(csp_bin_sem_handle_t * sem, CSP_BASE_TYPE * task_woken) {
    *task_woken = 0; 
    if (sem_post(sem) == 0) {
        return CSP_SEMAPHORE_OK;
    } else {
        return CSP_SEMAPHORE_ERROR;
    }
}
