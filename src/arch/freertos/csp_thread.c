

#include <FreeRTOS.h>
#include <task.h>

#include <csp/arch/csp_thread.h>

#include <FreeRTOS.h>
#include <task.h>

#if (configSUPPORT_DYNAMIC_ALLOCATION == 1)
int csp_freertos_thread_create(TaskFunction_t routine, const char * const thread_name, unsigned int stack_size, void * parameters, unsigned int priority, void ** return_handle) {

	TaskHandle_t handle;
#if (tskKERNEL_VERSION_MAJOR >= 8)
	portBASE_TYPE ret = xTaskCreate(routine, thread_name, stack_size, parameters, priority, &handle);
#else
	portBASE_TYPE ret = xTaskCreate(routine, (signed char *)thread_name, stack_size, parameters, priority, &handle);
#endif
	if (ret != pdTRUE) {
		return CSP_ERR_NOMEM;
	}
	if (return_handle) {
		*return_handle = handle;
	}
	return CSP_ERR_NONE;
}
#endif

// TODO xTaskCreateStatic

void csp_sleep_ms(unsigned int time_ms) {

	vTaskDelay(time_ms / portTICK_PERIOD_MS);
}
