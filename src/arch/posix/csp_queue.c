#include <pthread.h>

#include <csp/csp.h>
#include <csp/csp_queue.h>
#include <csp/csp_pthread_queue.h>

csp_queue_handle_t csp_queue_create(int length, size_t item_size) {
    return pthread_queue_create(length, item_size);
}

int csp_queue_enqueue(csp_queue_handle_t handle, void *value, int timeout) {
    return pthread_queue_enqueue(handle, value, timeout);
}

int csp_queue_enqueue_isr(csp_queue_handle_t handle, void * value, signed CSP_BASE_TYPE * task_woken) {
    task_woken = 0;
    return csp_queue_enqueue(handle, value, 0);
}

int csp_queue_dequeue(csp_queue_handle_t handle, void *buf, int timeout) {
    return pthread_queue_dequeue(handle, buf, timeout);
}

int csp_queue_dequeue_isr(csp_queue_handle_t handle, void *buf, signed CSP_BASE_TYPE * task_woken) {
    task_woken = 0;
    return csp_queue_dequeue(handle, buf, 0);
}

int csp_queue_size(csp_queue_handle_t handle) {
    return pthread_queue_size(handle);
}

int csp_queue_size_isr(csp_queue_handle_t handle) {
    return pthread_queue_size(handle);
}
