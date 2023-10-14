/****************************************************************************
 * **File:** csp/csp_interface.h
 *
 * **Description:** CSP Interface
 ****************************************************************************/
#pragma once

#include <csp/csp_types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CSP_IFLIST_NAME_MAX 10

/**
 * Interface Tx function.
 *
 * @return #CSP_ERR_NONE on success, otherwise an error code.
 */
typedef int (*nexthop_t)(csp_iface_t * iface, uint16_t via, csp_packet_t * packet, int from_me);

/**
 * This struct is referenced in documentation.
 * Update doc when you change this.
 */
struct csp_iface_s {

	/* Interface settings */
	uint16_t addr;              /**< Host address on this subnet */
	uint16_t netmask;           /**< Subnet mask */
	const char * name;          /**< Name, max compare length is #CSP_IFLIST_NAME_MAX */
	void * interface_data;      /**< Interface data, only known/used by the interface layer, e.g. state information. */
	void * driver_data;         /**< Driver data, only known/used by the driver layer, e.g. device/channel references. */
	nexthop_t nexthop;          /**< Next hop (Tx) function */
	uint8_t is_default;         /**< Set default IF flag (CSP supports multiple defaults) */

	/* Stats */
	uint32_t tx;                /**< Successfully transmitted packets */
	uint32_t rx;                /**< Successfully received packets */
	uint32_t tx_error;          /**< Transmit errors (packets) */
	uint32_t rx_error;          /**< Receive errors, e.g. too large message */
	uint32_t drop;              /**< Dropped packets */
	uint32_t autherr;           /**< Authentication errors (packets) */
	uint32_t frame;             /**< Frame format errors (packets) */
	uint32_t txbytes;           /**< Transmitted bytes */
	uint32_t rxbytes;           /**< Received bytes */
	uint32_t irq;               /**< Interrupts */

	/* For linked lists*/
	struct csp_iface_s * next;

};

/**
 * Inputs a new packet into the system.
 *
 * This function can be called from interface drivers (ISR) or tasks, to route and accept packets.
 *
 * .. note:: EXTREMELY IMPORTANT: \a pxTaskWoken must ALWAYS be NULL if called from task, and ALWAYS
 *			 be NON NULL if called from ISR. If this condition is met, this call is completely thread-safe
 *
 * This function is fire and forget, it returns void, meaning that the \a packet will always be
 * either accepted or dropped, so the memory will always be freed.
 *
 * @param[in] packet A pointer to the incoming packet
 * @param[in] iface A pointer to the incoming interface TX function.
 * @param[in] pxTaskWoken Valid reference if called from ISR, otherwise NULL!
 *
 */
void csp_qfifo_write(csp_packet_t * packet, csp_iface_t * iface, void * pxTaskWoken);

#ifdef __cplusplus
}
#endif
