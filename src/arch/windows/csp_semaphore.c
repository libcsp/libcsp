#include <Windows.h>
#include <csp/csp.h>
#include <csp/arch/csp_semaphore.h>

int csp_mutex_create(csp_mutex_t * mutex) {
    HANDLE mutexHandle = CreateMutex(NULL, FALSE, FALSE);
    if( mutexHandle == NULL ) {
        return CSP_MUTEX_ERROR;
    }
    *mutex = mutexHandle;
    return CSP_MUTEX_OK;
}

int csp_mutex_remove(csp_mutex_t * mutex) {
    if( !CloseHandle(*mutex) ) {
        return CSP_MUTEX_ERROR;
    }
    return CSP_MUTEX_OK;
}

int csp_mutex_lock(csp_mutex_t * mutex, uint32_t timeout) {
    if(WaitForSingleObject(*mutex, timeout) == WAIT_OBJECT_0) {
            return CSP_MUTEX_OK;
    }
    return CSP_MUTEX_ERROR;

}

int csp_mutex_unlock(csp_mutex_t * mutex) {
    if( !ReleaseMutex(*mutex) ) {
        return CSP_MUTEX_ERROR;
    }
    return CSP_MUTEX_OK;
}

int csp_bin_sem_create(csp_bin_sem_handle_t * sem) {
    HANDLE semHandle = CreateSemaphore(NULL, 1, 1, NULL);
    if( semHandle == NULL ) {
        return CSP_SEMAPHORE_ERROR;
    }
    *sem = semHandle;
    return CSP_SEMAPHORE_OK;
}

int csp_bin_sem_remove(csp_bin_sem_handle_t * sem) {
    if( !CloseHandle(*sem) ) {
        return CSP_SEMAPHORE_ERROR;
    }
    return CSP_SEMAPHORE_OK;
}

int csp_bin_sem_wait(csp_bin_sem_handle_t * sem, uint32_t timeout) {
    if( WaitForSingleObject(*sem, timeout) == WAIT_OBJECT_0 ) {
            return CSP_SEMAPHORE_OK;
    }
    return CSP_SEMAPHORE_ERROR;

}

int csp_bin_sem_post(csp_bin_sem_handle_t * sem) {
    if( !ReleaseSemaphore(*sem, 1, NULL) ) {
        return CSP_SEMAPHORE_ERROR;
    }
    return CSP_SEMAPHORE_OK;
}

int csp_bin_sem_post_isr(csp_bin_sem_handle_t * sem, CSP_BASE_TYPE * task_woken) {
    if( task_woken != NULL ) {
        *task_woken = 0;
    }
    return csp_bin_sem_post(sem);
}


