#include "csp_mutex.h"

void csp_mutex_init(csp_mutex_t * m) {
    m->handle = xSemaphoreCreateRecursiveMutexStatic(&m->buffer);
    configASSERT(m->handle);
}

void csp_mutex_lock(csp_mutex_t * m) {
    configASSERT(pdTRUE == xSemaphoreTakeRecursive(m->handle, portMAX_DELAY));
}

void csp_mutex_unlock(csp_mutex_t * m) {
    configASSERT(pdTRUE == xSemaphoreGiveRecursive(m->handle));
}
