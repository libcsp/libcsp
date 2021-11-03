#ifndef MESSAGE_BUFFER_DEFH
#define MESSAGE_BUFFER_DEFH

#include <FreeRTOS.h>
#include <os_task.h>
#include <os_timer.h>

struct message_buffer {
    int head;
    int tail;
    int len;
    uint8_t *fifo;
};

typedef struct message_buffer* MessageBufferHandle_t;

MessageBufferHandle_t xMessageBufferCreate(size_t xBufferSizeBytes);

size_t xMessageBufferSend(MessageBufferHandle_t xMessageBuffer,
                          const void *pvTxData,
                          size_t xDataLengthBytes,
                          TickType_t xTicksToWait);

size_t xMessageBufferReceive(MessageBufferHandle_t xMessageBuffer,
                             void *pvRxData,
                             size_t xBufferLengthBytes,
                             TickType_t xTicksToWait);

#endif /* MESSAGE_BUFFER_DEFH */
