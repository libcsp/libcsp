/****************************************************************************
 * **File:** csp/drivers/csp_socketcan.h
 *
 * **Description:** Socket CAN driver (Linux).
 *
 * .. note:: This driver requires the libsocketcan library.
 ****************************************************************************/
#pragma once

#include <csp/interfaces/csp_if_can.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Open CAN socket and add CSP interface.
 *
 * .. note:: With \a fd enabled, configuring the device requires libsocketcan
 *			 with CAN FD support (newer than v0.0.12). Otherwise pass bitrate 0
 *			 and configure the link before calling this function, e.g.:
 *			 ip link set can0 up type can bitrate 1000000 dbitrate 4000000 fd on restart-ms 100
 *
 * Parameters:
 * @param[in] device CAN device name (Linux device).
 * @param[in] ifname CSP interface name, use NULL for default name.
 * @param[in] node_id CSP address of the interface.
 * @param[in] bitrate if different from 0, it will be attempted to change the
 *            bitrate on the CAN device - this may require increased OS privileges.
 *            Must be 0 with \a fd enabled: the CAN FD profile is fixed
 *            (see csp_if_can.h) and is configured on the device when
 *            supported.
 * @param[in] fd open the device in CAN FD mode, using the fixed profile
 *            (requires CSP version 2 and a CAN FD enabled link).
 * @param[in] promisc if true, receive all CAN frames. If false a filter
 *                    is set on the CAN device, using device->addr
 * @param[out] return_iface the added interface.
 * @return #CSP_ERR_NONE on success, otherwise an error code.
 */
int csp_can_socketcan_open_and_add_interface(const char * device, const char * ifname, unsigned int node_id, int bitrate, bool fd, bool promisc, csp_iface_t ** return_iface);

/**
 * Initialize socketcan and add CSP interface.
 *
 * :bdg-warning-line:`deprecated` version 1.6, use csp_can_socketcan_open_and_add_interface()
 *
 * Parameters:
 * @param[in] device CAN device name (Linux device).
 * @param[in] node_id CSP address of the interface.
 * @param[in] bitrate if different from 0, it will be attempted to change the
 *            bitrate on the CAN device - this may require increased OS privileges.
 * @param[in] promisc if true, receive all CAN frames. If false a filter
 *                    is set on the CAN device, using device->addr
 * @return The added interface, or NULL in case of failure.
 */
csp_iface_t * csp_can_socketcan_init(const char * device, unsigned int node_id, int bitrate, bool promisc);

/**
 * Stop the Rx thread and free resources (testing).
 *
 * .. note:: This will invalidate CSP, because an interface can't be removed.
 *			 This is primarily for testing.
 *
 * Parameters:
 * @param[in] iface interface to stop.
 * @return #CSP_ERR_NONE on success, otherwise an error code.
 */
int csp_can_socketcan_stop(csp_iface_t * iface);

#ifdef __cplusplus
}
#endif
