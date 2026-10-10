/****************************************************************************
 * **File:** csp/interfaces/csp_if_canfd.h
 *
 * **Description:** CAN FD interface
 *
 * Transports CSP over CAN FD frames with up to 64 data bytes, using the same
 * CFP 2.x format as csp_if_can.h. Requires CSP version 2.
 *
 * Like classic CSP/CAN at 1 Mbit/s, CSP over CAN FD uses one fixed bus
 * configuration: 1 Mbit/s nominal and 4 Mbit/s data phase, ISO CAN FD with
 * Bit Rate Switch.
 *
 * CAN FD frames can only hold 0-8, 12, 16, 20, 24, 32, 48 or 64 data bytes,
 * other lengths are padded by the controller. CFP takes the packet length
 * from the frame lengths, so only these sizes are sent.
 *
 * A CAN FD interface sends and receives CAN FD frames only, and drivers drop
 * classic frames on it. Classic and CAN FD nodes can share a bus when the
 * classic controllers tolerate CAN FD frames, but cannot reach each other.
 * A node can open a classic and a CAN FD interface on one SocketCAN device,
 * with addresses in different subnets, to route between them.
 ****************************************************************************/
#pragma once

#include <csp/interfaces/csp_if_can.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Default interface name.
 */
#define CSP_IF_CANFD_DEFAULT_NAME "CANFD"

/**
 * Max number of data bytes in a single CAN FD frame.
 */
#define CSP_CANFD_FRAME_SIZE 64

/**
 * Fixed CAN FD nominal (arbitration) bitrate.
 */
#define CSP_CANFD_BITRATE 1000000

/**
 * Fixed CAN FD data phase bitrate.
 */
#define CSP_CANFD_DATA_BITRATE 4000000

/**
 * Add interface.
 *
 * Same interface data as classic CAN. The driver's tx_func gets up to
 * #CSP_CANFD_FRAME_SIZE bytes per frame (a byte count, not a DLC code) and
 * sends them as CAN FD frames with Bit Rate Switch.
 *
 * @param[in] iface CSP interface, initialized with name and inteface_data
 * 								pointing to a valid #csp_can_interface_data_t structure.
 * @return #CSP_ERR_NONE on success, #CSP_ERR_NOTSUP if not CSP version 2,
 *         otherwise an error code.
 */
int csp_canfd_add_interface(csp_iface_t * iface);

/**
 * Remove interface.
 *
 * @param[in] iface CSP interface to be removed.
 *
 * @return #CSP_ERR_NONE on success, otherwise an error code.
 */
int csp_canfd_remove_interface(csp_iface_t * iface);

/**
 * Process received CAN FD frame.
 *
 * Called from driver when a CAN FD frame has been received.
 * The function will gather the fragments into a single CSP packet and route
 * it on when complete.
 *
 * @param[in] iface incoming interface.
 * @param[in] id received CAN message identifier.
 * @param[in] data received CAN data.
 * @param[in] data_size number of received bytes (not the DLC code).
 * @param[in] timestamp_rx interface-specific RX timestamp. Only set for End frames
 * @param[out] pxTaskWoken Valid reference if called from ISR, otherwise NULL!
 * @return #CSP_ERR_NONE on success, otherwise an error code.
 */
int csp_canfd_rx(csp_iface_t * iface, uint32_t id, const uint8_t * data, uint8_t data_size, uint32_t timestamp_rx, int * pxTaskWoken);

#ifdef __cplusplus
}
#endif
