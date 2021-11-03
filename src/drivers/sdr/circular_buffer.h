#ifndef CIRCULAR_BUFFER_DEFH
#define CIRCULAR_BUFFER_DEFH

#include <FreeRTOS.h>
#include <os_task.h>
#include <os_timer.h>

#ifdef __cplusplus
extern "C" {
#endif

/* A producer/consumer API that uses a circular buffer of fixed size objects.
 * The implementation uses no locks, and is therefore only concurrency safe
 * for a single producer and consumer.
 */
struct circular_buffer;
typedef struct circular_buffer circular_buffer_t;
typedef struct circular_buffer* CircularBufferHandle;

CircularBufferHandle CircularBufferCreate(size_t ElementSize, int NumElements);

/* NextHead returns a pointer to the next producer element. The method returns
 * NULL if the circular buffer is full.
 */
void* CircularBufferNextHead(CircularBufferHandle);
/* Send exposes the new head to the consumer. Once this function has been called
 * the producer should not touch the element contents.
 */
void CircularBufferSend(CircularBufferHandle);

/* NextTail returns a pointer to the next consumer element. The method returns
 * NULL if the circular buffer is empty.
 */
void* CircularBufferNextTail(CircularBufferHandle);
/* Receive exposes the new tail to the producer. The consumer should not touch
 * element contents after this call has been made.
 */ 
void CircularBufferReceive(CircularBufferHandle);

#ifdef __cplusplus
}
#endif
#endif /* MESSAGE_BUFFER_DEFH */
