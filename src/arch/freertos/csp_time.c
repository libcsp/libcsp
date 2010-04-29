#include <stdint.h>

/* FreeRTOS includes */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

/* CSP includes */
#include <csp/csp.h>
#include <csp/csp_time.h>

uint32_t csp_get_ms() {
    return (uint32_t)(xTaskGetTickCount() * (1000/configTICK_RATE_HZ));
}
