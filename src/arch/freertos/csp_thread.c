#include <FreeRTOS.h>
#include <task.h>

void csp_sleep_ms(unsigned int time_ms) {

	vTaskDelay(time_ms / portTICK_PERIOD_MS);
}
