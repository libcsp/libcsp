/****************************************************************************
 * **File:** csp/interfaces/csp_if_kiss.h
 *
 * **Description:** KISS interface (serial).
 ****************************************************************************/
#pragma once

#include <csp/csp_interface.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Default name of KISS interface.
 */
#define CSP_IF_KISS_DEFAULT_NAME "KISS"

/**
 * Send KISS frame (implemented by driver).
 *
 * @param[in] driver_data driver data from #csp_iface_t
 * @param[in] data data to send
 * @param[in] len length of \a data.
 * @return #CSP_ERR_NONE on success, otherwise an error code.
 */
typedef int (*csp_kiss_driver_tx_t)(void *driver_data, const uint8_t * data, size_t len);

/**
 * KISS Rx mode/state.
 */
typedef enum {
	KISS_MODE_NOT_STARTED,  /**< No start detected */
	KISS_MODE_STARTED,      /**< Started on a KISS frame */
	KISS_MODE_ESCAPED,      /**< Rx escape character */
	KISS_MODE_SKIP_FRAME,   /**< Skip remaining frame, wait for end character */
} csp_kiss_mode_t;

/**
 * KISS interface data (state information).
 */
typedef struct {
	csp_kiss_driver_tx_t tx_func; /**< Tx function */
	csp_kiss_mode_t rx_mode; /**< Rx mode/state. */
	unsigned int rx_length; /**< Rx length */
	bool rx_first; /**< Rx first - if set, waiting for first character
						(== TNC_DATA) after start */
	csp_packet_t * rx_packet; /**< CSP packet for storing Rx data. */
} csp_kiss_interface_data_t;

/**
 * Add interface.
 *
 * @param[in] iface CSP interface, initialized with name and
 * 								inteface_data pointing to a valid #csp_kiss_interface_data_t.
 * @return #CSP_ERR_NONE on success, otherwise an error code.
 */
int csp_kiss_add_interface(csp_iface_t * iface);

/**
 * Send CSP packet over KISS (nexthop).
 *
 * @return #CSP_ERR_NONE on success, otherwise an error code.
 */
int csp_kiss_tx(csp_iface_t * iface, uint16_t via, csp_packet_t * packet, int from_me);

/**
 * Process received CAN frame.
 *
 * Called from driver when a chunk of data has been received. Once a complete
 * frame has been received, the CSP packet will be routed on.
 *
 * @param[in] iface incoming interface.
 * @param[in] buf reveived data.
 * @param[in] len length of \a buf.
 * @param[out] pxTaskWoken Valid reference if called from ISR, otherwise NULL!
 */
void csp_kiss_rx(csp_iface_t * iface, const uint8_t * buf, size_t len, void * pxTaskWoken);

#ifdef __cplusplus
}
#endif
