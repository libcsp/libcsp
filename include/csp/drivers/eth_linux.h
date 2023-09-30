/****************************************************************************
 * **File:** drivers/eth_linux.h
 *
 * **Description:** Posix ETH driver
 *
 * .. note:: Using this driver require user elevation. Guideline for doing this
 *           is given at run-time
 *
 ****************************************************************************/
#pragma once

#include <csp/interfaces/csp_if_eth.h>

/**
 * Open RAW socket and add CSP interface.
 *
 * Parameters:
 *	device (const char *) [in]: network interface name (Linux device).
 *	ifname (const char *) [in]: ifname CSP interface name.
 *	mtu (int) [in]: MTU for the transmitted ethernet frames.
 *	node_id (unsigned int) [in]: CSP address of the interface.
 *	promisc (bool) [in]: if true, receive all CAN frames. If false a filter is set before forwarding packets to the router
 *	return_iface (csp_iface_t **) [out]: the added interface.
 *
 * Returns:
 *	int: #CSP_ERR_NONE on success, otherwise an error code.
 */
int csp_eth_init(const char * device, const char * ifname, int mtu, unsigned int node_id, bool promisc, csp_iface_t ** return_iface);

/**
 * Transmit an CSP ethernet frame
 *
 * Parameters:
 *	iface (csp_iface_t *) [in]: Ethernet interface to use.
 *	eth_frame (csp_eth_header_t *) [in]: The CSP ethernet frame to transmit.
 *
 * Returns:
 * 	int: #CSP_ERR_NONE on success, otherwise an error code.
 */
int csp_eth_tx_frame(csp_iface_t * iface, csp_eth_header_t * eth_frame);

/**
 * Posix ethernet RX thread
 *
 * Parameters:
 *	param (void *) [in]:Ethernet CSP interface to use.
 *
 * Returns:
 *	void *: NULL
 */
void * csp_eth_rx_loop(void * param);
