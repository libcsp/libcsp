#include <string.h>
#include <csp/arch/csp_malloc.h>
#include "circular_buffer.h"
#include "util/service_utilities.h"

struct circular_buffer {
    int head;
    int tail;
    int num_elements;
    size_t element_size;
    uint8_t *buf;
};

CircularBufferHandle CircularBufferCreate(size_t ElementSize, int NumElements) {
    CircularBufferHandle cbuf = csp_calloc(1, sizeof(circular_buffer_t));

    if (!cbuf)
        return 0;

    cbuf->buf = csp_malloc(ElementSize * NumElements);
    if (!cbuf->buf) {
        csp_free(cbuf);
        return 0;
    }
    cbuf->element_size = ElementSize;
    cbuf->num_elements = NumElements;
    return cbuf;
}

static inline int advance_head(CircularBufferHandle cbuf) {
    int head = cbuf->head;
    ++head; // advance to next available slot
    if (head >= cbuf->num_elements)
        head = 0; // wrap if necessary
    if (head == cbuf->tail)
        return -1; // all full!
    return head;
}

void* CircularBufferNextHead(CircularBufferHandle cbuf)
{
    int head = advance_head(cbuf);
    return (head >= 0) ? cbuf->buf + head*cbuf->element_size : 0;
}

void CircularBufferSend(CircularBufferHandle cbuf) {
    int head = advance_head(cbuf);
    if (head < 0) {
        ex2_log("queue full!");
        return;
    }
    cbuf->head = head; // make the new head visible to the consumer
}

static inline int advance_tail(CircularBufferHandle cbuf) {
    int tail = cbuf->tail;
    if (tail == cbuf->head)
        return -1;  // fifo is empty
    ++tail; // next producer element
    if (tail >= cbuf->num_elements)
        tail = 0; // wrap if necessary
    return tail;
}

void* CircularBufferNextTail(CircularBufferHandle cbuf) {
    int tail = advance_tail(cbuf);
    return (tail >= 0) ? cbuf->buf + tail*cbuf->element_size : 0;
}

void CircularBufferReceive(CircularBufferHandle cbuf) {
    int tail = advance_tail(cbuf);
    if (tail < 0) {
        ex2_log("queue empty!");
        return;
    }
    cbuf->tail = tail; // make the new tail visible to the producer
}

