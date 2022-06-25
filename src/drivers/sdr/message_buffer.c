#include <string.h>
#include <csp/arch/csp_malloc.h>
#include "message_buffer.h"

MessageBufferHandle_t xMessageBufferCreate(size_t xBufferSizeBytes) {
    MessageBufferHandle_t msgbuf = csp_calloc(1, sizeof(struct message_buffer));

    if (msgbuf) {
        msgbuf->fifo = csp_malloc(xBufferSizeBytes);
        if (!msgbuf->fifo) {
            csp_free(msgbuf);
            return 0;
        }
        msgbuf->len = xBufferSizeBytes;
    }

    return msgbuf;
}

size_t xMessageBufferSend(MessageBufferHandle_t xMessageBuffer,
                          const void *pvTxData,
                          size_t xDataLengthBytes,
                          TickType_t xTicksToWait) {
    uint8_t *start = xMessageBuffer->fifo;
    size_t data_len = xDataLengthBytes + sizeof(size_t);

    if (xMessageBuffer->len % data_len != 0) {
        ex2_log("Warning: buffer length is not an even multiple of data length");
    }
    if (xMessageBuffer->head >= xMessageBuffer->tail) {
        // head is ahead of tail, just need to worry about wrap
        size_t dist = xMessageBuffer->len - xMessageBuffer->head;
        if (dist - data_len > 0) {
            // there is room before we have to wrap, so start = head
            start += xMessageBuffer->head;
        }
    }
    else {
        // head is behind tail - make sure there is room
        size_t dist = xMessageBuffer->tail - xMessageBuffer->head;
        if (dist - data_len > 0) {
            // there is room before the tail, so start = head
            start += xMessageBuffer->head;
        }
        else { // head would overtake tail :-(
            return 0;
        }
    }
    /* Write the length to be compatible with FreeRTOS. Note length includes
     * the length field itself.
     */
    memcpy(start, &data_len, sizeof(data_len));
    memcpy(start + sizeof(data_len), pvTxData, xDataLengthBytes);
    xMessageBuffer->head = (start - xMessageBuffer->fifo) + data_len;

    return data_len;
}

size_t xMessageBufferReceive(MessageBufferHandle_t xMessageBuffer,
                             void *pvRxData,
                             size_t xBufferLengthBytes,
                             TickType_t xTicksToWait) {
    if (xMessageBuffer->tail == xMessageBuffer->head) {
        return 0;
    }
    size_t total_len = 0;
    uint8_t *start = xMessageBuffer->fifo + xMessageBuffer->tail;
    memcpy(&total_len, start, sizeof(total_len));
    size_t data_len = total_len - sizeof(total_len);
    if (data_len > xBufferLengthBytes)
        data_len = xBufferLengthBytes;
    memcpy(pvRxData, start + sizeof(size_t), data_len);

    if (xMessageBuffer->tail + total_len >= xMessageBuffer->len) {
        xMessageBuffer->tail = 0;
    }
    else {
        xMessageBuffer->tail = xMessageBuffer->tail + total_len;
    }
    return total_len - sizeof(total_len);
}
