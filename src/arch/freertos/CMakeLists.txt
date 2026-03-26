target_sources(csp_es PRIVATE
  csp_clock.c
  csp_queue.c
  csp_semaphore.c
  csp_system.c
  csp_time.c
  csp_hooks.c
)

# Check if freertos_kernel target exists (provided by host project)
if(NOT TARGET freertos_kernel)
  # Create a dummy freertos_kernel target for standalone builds
  message(STATUS "freertos_kernel target not found, creating dummy target for build testing")
  add_library(freertos_kernel INTERFACE)

  # Create minimal FreeRTOS stub headers for build testing
  set(FREERTOS_STUB_DIR "${CMAKE_CURRENT_BINARY_DIR}/freertos_stub")
  file(MAKE_DIRECTORY "${FREERTOS_STUB_DIR}")

  # Create minimal FreeRTOS.h stub
  file(WRITE "${FREERTOS_STUB_DIR}/FreeRTOS.h" "
#ifndef FREERTOS_H
#define FREERTOS_H

/* Minimal FreeRTOS stub for build testing */
#include <stdint.h>
#include <stddef.h>

/* Basic FreeRTOS types */
typedef void* QueueHandle_t;
typedef void* SemaphoreHandle_t;
typedef void* TaskHandle_t;
typedef uint32_t TickType_t;
typedef uint32_t BaseType_t;
typedef uint32_t UBaseType_t;
typedef BaseType_t portBASE_TYPE;

/* Static allocation types */
typedef struct {
    uint8_t dummy[64]; /* Placeholder for static queue storage */
} StaticQueue_t;

#define pdTRUE  1
#define pdFALSE 0
#define pdPASS  1
#define pdFAIL  0

#define portMAX_DELAY 0xFFFFFFFF
#define portTICK_PERIOD_MS 1
#define configTICK_RATE_HZ 1000

/* Stub functions - these would normally be implemented by FreeRTOS */
static inline QueueHandle_t xQueueCreate(UBaseType_t uxQueueLength, UBaseType_t uxItemSize) { return NULL; }
static inline QueueHandle_t xQueueCreateStatic(UBaseType_t uxQueueLength, UBaseType_t uxItemSize, uint8_t *pucQueueStorage, StaticQueue_t *pxStaticQueue) { return NULL; }
static inline BaseType_t xQueueSend(QueueHandle_t xQueue, const void *pvItemToQueue, TickType_t xTicksToWait) { return pdFAIL; }
static inline BaseType_t xQueueSendToBack(QueueHandle_t xQueue, const void *pvItemToQueue, TickType_t xTicksToWait) { return pdFAIL; }
static inline BaseType_t xQueueSendToBackFromISR(QueueHandle_t xQueue, const void *pvItemToQueue, BaseType_t *pxHigherPriorityTaskWoken) { return pdFAIL; }
static inline BaseType_t xQueueReceive(QueueHandle_t xQueue, void *pvBuffer, TickType_t xTicksToWait) { return pdFAIL; }
static inline BaseType_t xQueueReceiveFromISR(QueueHandle_t xQueue, void *pvBuffer, BaseType_t *pxHigherPriorityTaskWoken) { return pdFAIL; }
static inline UBaseType_t uxQueueMessagesWaiting(QueueHandle_t xQueue) { return 0; }
static inline UBaseType_t uxQueueMessagesWaitingFromISR(QueueHandle_t xQueue) { return 0; }
static inline UBaseType_t uxQueueSpacesAvailable(QueueHandle_t xQueue) { return 0; }
static inline void xQueueReset(QueueHandle_t xQueue) { }
static inline void vQueueDelete(QueueHandle_t xQueue) { }

static inline SemaphoreHandle_t xSemaphoreCreateMutex(void) { return NULL; }
static inline BaseType_t xSemaphoreTake(SemaphoreHandle_t xSemaphore, TickType_t xTicksToWait) { return pdFAIL; }
static inline BaseType_t xSemaphoreGive(SemaphoreHandle_t xSemaphore) { return pdFAIL; }
static inline void vSemaphoreDelete(SemaphoreHandle_t xSemaphore) { }

static inline TickType_t xTaskGetTickCount(void) { return 0; }
static inline TickType_t xTaskGetTickCountFromISR(void) { return 0; }
static inline void vTaskDelay(TickType_t xTicksToDelay) { }
static inline TaskHandle_t xTaskGetCurrentTaskHandle(void) { return NULL; }
static inline uint32_t ulTaskNotifyTake(BaseType_t xClearCountOnExit, TickType_t xTicksToWait) { return 0; }
static inline BaseType_t xTaskNotifyGive(TaskHandle_t xTaskToNotify) { return pdFAIL; }

#endif /* FREERTOS_H */
")

  # Create other required FreeRTOS headers
  file(WRITE "${FREERTOS_STUB_DIR}/queue.h" "
#ifndef QUEUE_H
#define QUEUE_H
#include \"FreeRTOS.h\"
#endif /* QUEUE_H */
")

  file(WRITE "${FREERTOS_STUB_DIR}/semphr.h" "
#ifndef SEMPHR_H
#define SEMPHR_H
#include \"FreeRTOS.h\"
#endif /* SEMPHR_H */
")

  file(WRITE "${FREERTOS_STUB_DIR}/task.h" "
#ifndef TASK_H
#define TASK_H
#include \"FreeRTOS.h\"
#endif /* TASK_H */
")

  # Add the stub directory to the include path
  target_include_directories(freertos_kernel INTERFACE "${FREERTOS_STUB_DIR}")

  message(STATUS "Created FreeRTOS stub headers in: ${FREERTOS_STUB_DIR}")
endif()

target_link_libraries(csp_es PRIVATE freertos_kernel)
