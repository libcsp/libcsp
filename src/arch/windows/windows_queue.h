#pragma once

#include <csp/arch/csp_queue.h>

#define WINDOWS_QUEUE_ERROR CSP_QUEUE_ERROR
#define WINDOWS_QUEUE_EMPTY CSP_QUEUE_ERROR
#define WINDOWS_QUEUE_FULL  CSP_QUEUE_ERROR
#define WINDOWS_QUEUE_OK    CSP_QUEUE_OK

typedef struct windows_queue_s windows_queue_t;

windows_queue_t * windows_queue_create(int length, size_t item_size);
void windows_queue_delete(windows_queue_t * q);
int windows_queue_enqueue(windows_queue_t * queue, const void * value, int timeout);
int windows_queue_dequeue(windows_queue_t * queue, void * buf, int timeout);
int windows_queue_items(windows_queue_t * queue);
