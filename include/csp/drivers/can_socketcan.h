/****************************************************************************
 * File: csp_socketcan.h
 * Description: Socket CAN driver (Linux).
 * This driver requires the libsocketcan library.
 ****************************************************************************/
#pragma once

#include <csp/interfaces/csp_if_can.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Open CAN socket and add CSP interface.
 *
 * Parameters:
 *	device (const char *) [in]: CAN device name (Linux device).
 *	ifname (const char *) [in]: CSP interface name, use #CSP_IF_CAN_DEFAULT_NAME for default name.
 *	node_id (unsigned int) [in]: CSP address of the interface.
 *	bitrate (int) [in]: if different from 0, it will be attempted to change the bitrate on the CAN device - this may require increased OS privileges.
 *	promisc (bool) [in]: if \a true, receive all CAN frames. If \a false a filter is set on the CAN device, using device->addr
 *	return_iface (csp_iface_t **) [out]: the added interface.
 *
 * Returns:
 *	int: The added interface, or NULL in case of failure.
*/
int csp_can_socketcan_open_and_add_interface(const char * device, const char * ifname, unsigned int node_id, int bitrate, bool promisc, csp_iface_t ** return_iface);

/**
 * Initialize socketcan and add CSP interface.
 *
 * :bdg-warning-line:`deprecated` version 1.6, use csp_can_socketcan_open_and_add_interface()
 *
 * Parameters:
 *	device (const char *) [in]: CAN device name (Linux device).
 *	node_id (unsigned int) [in]: CSP address of the interface.
 *	bitrate (int) [in]: if different from 0, it will be attempted to change the bitrate on the CAN device - this may require increased OS privileges.
 *	promisc (bool) [in]: if \a true, receive all CAN frames. If \a false a filter is set on the CAN device, using device->addr
 *
 * Returns:
 *	csp_iface_t *: The added interface, or NULL in case of failure.
*/
csp_iface_t * csp_can_socketcan_init(const char * device, unsigned int node_id, int bitrate, bool promisc);

/**
 * Stop the Rx thread and free resources (testing).
 *
 * .. note:: This will invalidate CSP, because an interface can't be removed.
 *			 This is primarily for testing.
 *
 * Parameters:
 *	iface (csp_iface_t *) [in]: interface to stop.
 *
 * Returns:
 *	int: #CSP_ERR_NONE on success, otherwise an error code.
 */
int csp_can_socketcan_stop(csp_iface_t * iface);

#ifdef __cplusplus
}
#endif
