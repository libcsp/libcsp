#pragma once

/**
 *
 *  @file
 *
 *  CAN driver (Zephyr).
 *
 *  @note This driver requires the CAN driver for target board.
*/

#include <csp/interfaces/csp_if_can.h>
#include <zephyr/device.h>
#include <zephyr/drivers/can.h>

/**
 * Open CAN and add CSP interface.
 *
 *  @param[in] device CAN device structure.
 *  @param[in] ifname CSP interface name.
 *  @param[in] address CSP address of the interface.
 *  @param[in] bitrate CAN bitrate.
 *  @param[in] filter_addr Destination address you want to set in the RX filter.
 *  @param[in] filter_mask Bit mask you want to set in the RX filter.
 *  @param[out] return_iface Added interface
 *  @return #CSP_ERR_NONE on success, otherwise an error code.
*/
int csp_can_open_and_add_interface(const struct device * device, const char * ifname,
								   uint16_t address, uint32_t bitrate,
								   uint16_t filter_addr, uint16_t filter_mask,
								   csp_iface_t ** return_iface);

/**
 * Set CAN RX filter based on destination address and bit mask.
 *
 *  @param[in] iface Interface to set the RX filter.
 *  @param[in] filter_addr Destination address you want to set in the RX filter.
 *  @param[in] filter_mask Bit mask you want to set in the RX filter.
 *  @return Filter ID, a negative value on failure.
*/
int csp_can_set_rx_filter(csp_iface_t * iface, uint16_t filter_addr, uint16_t filter_mask);

/**
 * Stop the CAN and RX thread
 *
 *  @param[in] iface Interface to stop.
 *  @return #CSP_ERR_NONE on success, otherwise an error code.
*/
int csp_can_stop(csp_iface_t * iface);
