#ifndef _WINDOWS_QUEUE_H_
#define _WINDOWS_QUEUE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <Windows.h>
#include "windows_glue.h"
#undef interface

#include <csp/arch/csp_queue.h>

#define WINDOWS_QUEUE_ERROR CSP_QUEUE_ERROR
#define WINDOWS_QUEUE_EMPTY CSP_QUEUE_ERROR
#define WINDOWS_QUEUE_FULL CSP_QUEUE_ERROR
#define WINDOWS_QUEUE_OK CSP_QUEUE_OK

typedef struct windows_queue_s {
    void * buffer;
    int size;
    int item_size;
    int items;
    int head_idx;
    CRITICAL_SECTION mutex;
    CONDITION_VARIABLE cond_full;
    CONDITION_VARIABLE cond_empty;
} windows_queue_t;

windows_queue_t * windows_queue_create(int length, size_t item_size);
void windows_queue_delete(windows_queue_t * q);
int windows_queue_enqueue(windows_queue_t * queue, void * value, int timeout);
int windows_queue_dequeue(windows_queue_t * queue, void * buf, int timeout);
int windows_queue_items(windows_queue_t * queue);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // _WINDOWS_QUEUE_H_

