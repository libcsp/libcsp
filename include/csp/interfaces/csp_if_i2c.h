/****************************************************************************
 * **File:** csp/interfaces/csp_if_i2c.h
 *
 * **Description:** I2C interface.
 ****************************************************************************/
#pragma once

#include <csp/csp_interface.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Default name of I2C interface.
 */
#define CSP_IF_I2C_DEFAULT_NAME "I2C"

/**
 * Send I2C frame (implemented by driver).
 *
 * Used by csp_i2c_tx() to send a frame.
 *
 * The function must free the frame/packet using csp_buffer_free(),
 * if the send succeeds (returning #CSP_ERR_NONE).
 *
 * @param[in] driver_data driver data from #csp_iface_t
 * @param[in] frame destination, length and data. This is actually
 * 			  a #csp_packet_t buffer, casted to #csp_i2c_frame_t.
 *
 * @return #CSP_ERR_NONE on success, or an error code.
 */
typedef int (*csp_i2c_driver_tx_t)(void * driver_data, csp_packet_t * frame);

/**
 * Interface data (state information).
 */
typedef struct {
	csp_i2c_driver_tx_t tx_func; /**< Tx function */
} csp_i2c_interface_data_t;

/**
 * Add interface.
 *
 * @param[in] iface CSP interface, initialized with name and
 * 								inteface_data pointing to a valid #csp_i2c_interface_data_t.
 * @return #CSP_ERR_NONE on success, otherwise an error code.
 */
int csp_i2c_add_interface(csp_iface_t * iface);

/**
 * Send CSP packet over I2C (nexthop).
 *
 * @return #CSP_ERR_NONE on success, otherwise an error code.
 */
int csp_i2c_tx(csp_iface_t * iface, uint16_t via, csp_packet_t * packet, int from_me);

/**
 * Process received I2C frame.
 *
 * @note The received #csp_i2c_frame_t must actually be pointing to an #csp_packet_t.
 *
 * Called from driver, when a frame has been received.
 *
 * @param[in] iface incoming interface.
 * @param[in] frame received data, routed on as a #csp_packet_t.
 * @param[out] pxTaskWoken Valid reference if called from ISR, otherwise NULL!
 */
void csp_i2c_rx(csp_iface_t * iface, csp_packet_t * frame, void * pxTaskWoken);

#ifdef __cplusplus
}
#endif
