/**
 * @author Johan de Claville Christiansen
 * @copyright Space Inventor ApS (c) 2021
 * 
 * This is an example CAN driver for a Microchip ATSAMC21
 * The driver relies on FreeRTOS and the ASF4 board support package
 * and therefore cannot be built into CSP.
 * 
 * The driver is simply hardcoded for CAN0 device on those chips
 * The interface definition is a static variable in .data
 * it has two modes of input processing, ISR or DEFERRED
 *   ISR uses time in the locked part of the ISR
 *   DEFERRED trades time for memory. As it requires a task and stack.
 * 
 * We use the ISR mode on all systems, as the CFP processing is fast
 * and it has not proven a bottleneck in any of our systems.
 * 
 * The driver has 4 different receive options:
 *   1) Direct host match
 *   2) Match on L3 broadcast address (addr, netmask)
 *   3) Match on L2 broadcast address (0x3fff)
 *   4) Promiscious mode (match all)
 * 
 */

#include <csp/csp_debug.h>
#include <driver_init.h>
#include <hal_can_async.h>

#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>

#include <csp/csp.h>
#include <csp/interfaces/csp_if_can.h>

#include <drivers/csp_driver_can.h>
#include <drivers/sn65hvd230.h>

#ifndef CAN_DEFER_TASK
#define CAN_DEFER_TASK 0
#endif

#if CAN_DEFER_TASK
static StaticTask_t can_task_tcb;
static StackType_t can_task_stack[500];
static TaskHandle_t can_task_handle;
#endif

/** Driver configration */
static struct mcan_s {
	can_mode_e mode;
	uint32_t id;
    uint32_t id_l3bc;
    uint32_t id_l2bc;
	uint32_t mask;
	csp_can_interface_data_t ifdata;
	SemaphoreHandle_t lock;
	StaticSemaphore_t lock_buf;
	csp_iface_t interface;
} mcan[1] = {
	{
		.mode = 0,
		.lock = NULL,
		.interface = {
			.name = "CAN",
			.interface_data = &mcan[0].ifdata,
			.driver_data = &mcan[0],
		},
	}
};

void CAN_0_rx_callback(struct can_async_descriptor *const descr) {

    int xTaskWoken = pdFALSE;
	struct can_message msg;
	uint8_t data[8];
	msg.data = data;

	while(can_async_read(descr, &msg) == ERR_NONE) {

		//csp_hex_dump("rx", msg.data, msg.len);

		/* Process frame within ISR
		 * (This can also be deferred to a task with: csp_can_process_frame_deferred) */
		csp_can_rx(&mcan[0].interface, msg.id, msg.data, msg.len, &xTaskWoken);

	}

	portYIELD_FROM_ISR(xTaskWoken);

	return;
}

#if CAN_DEFER_TASK
void CAN_0_rx_callback_task(struct can_async_descriptor *const descr) {
	BaseType_t xTaskWoken = pdFALSE;
	vTaskNotifyGiveFromISR(can_task_handle, &xTaskWoken);
	portYIELD_FROM_ISR(xTaskWoken);
}

static void can_task(void * param) {
	
	while(1) {

		ulTaskNotifyTake(true, 1 * configTICK_RATE_HZ);

		int xTaskWoken = pdFALSE;
		struct can_message msg;
		uint8_t data[8];
		msg.data = data;

		while(can_async_read(&CAN_0, &msg) == ERR_NONE) {

			//csp_hex_dump("rx", msg.data, msg.len);

			/* Process frame within ISR
			* (This can also be deferred to a task with: csp_can_process_frame_deferred) */
			csp_can_rx(&mcan[0].interface, msg.id, msg.data, msg.len, &xTaskWoken);

		}

		if (xTaskWoken) {
			portYIELD();
		}

	}

}
#endif

void CAN_0_irq_callback(struct _can_async_device *dev, enum can_async_interrupt_type type) {
	can_async_disable(&CAN_0);
	mcan[0].interface.tx_error++;
	can_async_enable(&CAN_0);
	return;
}

int csp_can_tx_frame(void *driver_data, uint32_t id, const uint8_t * data, uint8_t dlc) {

	struct mcan_s * driver = (struct mcan_s *)driver_data;

	if ((driver == NULL) || (driver->lock == NULL)) {
		return 0;
	}

	struct can_message msg;
	msg.id = id;
	msg.type = CAN_TYPE_DATA;
	msg.data = data;
	msg.len = dlc;
	msg.fmt  = CAN_FMT_EXTID;

	/* Task locking */
	while(xSemaphoreTake(driver->lock, 1) == pdFALSE);

	/**
	 * Blocking IO:
	 *
	 * With CAN blocking IO is needed because we only have room for 32 frames or 256 bytes.
	 * This is barely enough to hold a large CSP packet
	 * We could go for async transmission with software queues, this will allow for larger amounts
	 * of data to be queued. However in this case, simplicity is chosen over performance.
	 * When the TX FIFO is full, we ask the task to sleep a tick.
	 * Usually data heavy fuctions are running in their separate tasks anyways.
	 * A CAN frame is 96 bits and is transmitted within 96 us.
	 * A tick period can vary from 1 to 10 ms (typically)
	 * 3 retries have been chosen because a futher dealy than this would almost certainly be
	 * due to an error.
	 */
	int attempts = 3;
	while((attempts > 0) && (can_async_write(&CAN_0, &msg) != ERR_NONE)) {
		vTaskDelay(1);
		attempts--;
	}

	/* Unlock */
	xSemaphoreGive(driver->lock);

	if (attempts > 0) {
		return CSP_ERR_NONE;
	} else {
		return CSP_ERR_BUSY;
	}
}


csp_iface_t * csp_driver_can_init(int addr, int netmask, int id, can_mode_e mode, uint32_t bitrate) {

	/* Create a mutex type semaphore. */
	mcan[id].lock = xSemaphoreCreateMutexStatic(&mcan[id].lock_buf);

	const csp_conf_t *csp_conf = csp_get_conf();

	/* Generate ID */
    uint32_t can_mask = 0;
    if (csp_conf->version == 2) {
        
        can_mask = CFP2_DST_MASK << CFP2_DST_OFFSET;

        mcan[id].id = addr << CFP2_DST_OFFSET;
        mcan[id].id_l3bc = ((1 << (csp_id_get_host_bits() - netmask)) - 1) << CFP2_DST_OFFSET;
        mcan[id].id_l2bc = 0x3FFF << CFP2_DST_OFFSET;
    } else {

        can_mask = CFP_MAKE_DST((1 << CFP_HOST_SIZE) - 1);

        mcan[id].id = CFP_MAKE_DST(addr);
        mcan[id].id_l2bc = 0; //! Not supported on CSP1
        mcan[id].id_l3bc = 0; //! Not supported on CSP1
    }

	if (mode == CSP_CAN_MASKED) {
		mcan[id].mask = can_mask;
	} else if (mode == CSP_CAN_PROMISC) {
		mcan[id].mask = 0;
	} else {
		csp_dbg_errno = CSP_DBG_ERR_INVALID_CAN_CONFIG;
		return NULL;
	}

	mcan[id].ifdata.tx_func = csp_can_tx_frame;
	mcan[id].ifdata.pbufs = NULL;
	mcan[id].interface.interface_data = &mcan[id].ifdata;

	mcan[id].interface.addr = addr;
	mcan[id].interface.netmask = netmask;

	/* Regsiter interface */
	csp_can_add_interface(&mcan[id].interface);

#if CAN_DEFER_TASK
	/* CAN task */
	can_async_register_callback(&CAN_0, CAN_ASYNC_RX_CB, (FUNC_PTR)CAN_0_rx_callback_task);
	can_task_handle = xTaskCreateStatic(can_task, "CAN", 500, NULL, 1, can_task_stack, &can_task_tcb);

#else

	/* CAN ISR */
	can_async_register_callback(&CAN_0, CAN_ASYNC_RX_CB, (FUNC_PTR)CAN_0_rx_callback);
#endif

	can_async_register_callback(&CAN_0, CAN_ASYNC_IRQ_CB, (FUNC_PTR)CAN_0_irq_callback);
	can_async_enable(&CAN_0);

    /* Filter for own address */
    struct can_filter filter;
    filter.id   = mcan[id].id;
    filter.mask = mcan[id].mask;
    //csp_print("H filter %x %x\n", filter.id, filter.mask);
    can_async_set_filter(&CAN_0, 0, CAN_FMT_EXTID, &filter);

    /* Filter for subnet broadcast address */
    filter.id   = mcan[id].id_l3bc;
    filter.mask = mcan[id].mask;
    //csp_print("L3 filter %x %x\n", filter.id, filter.mask);
    can_async_set_filter(&CAN_0, 1, CAN_FMT_EXTID, &filter);

    /* Filter for layer-2 broadcast */
    filter.id   = mcan[id].id_l2bc;
    filter.mask = mcan[id].mask;
    //csp_print("L2 filter %x %x\n", filter.id, filter.mask);
    can_async_set_filter(&CAN_0, 2, CAN_FMT_EXTID, &filter);

	return &mcan[id].interface;

}
