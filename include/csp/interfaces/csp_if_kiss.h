/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2012 GomSpace ApS (http://www.gomspace.com)
Copyright (C) 2012 AAUSAT3 Project (http://aausat3.space.aau.dk)

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef CSP_INTERFACES_CSP_IF_KISS_H
#define CSP_INTERFACES_CSP_IF_KISS_H

/**
   @file

   KISS interface (serial).
*/

#include <csp/csp_interface.h>
#include <csp/arch/csp_semaphore.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
   Default name of KISS interface.
*/
#define CSP_IF_KISS_DEFAULT_NAME "KISS"

/**
   Send KISS frame (implemented by driver).

   @param[in] driver_data driver data from #csp_iface_t
   @param[in] data data to send
   @param[in] len length of \a data.
   @return #CSP_ERR_NONE on success, otherwise an error code.
*/
typedef int (*csp_kiss_driver_tx_t)(void *driver_data, const uint8_t * data, size_t len);

/**
   KISS Rx mode/state.
*/
typedef enum {
	KISS_MODE_NOT_STARTED,  //!< No start detected
	KISS_MODE_STARTED,      //!< Started on a KISS frame
	KISS_MODE_ESCAPED,      //!< Rx escape character 
	KISS_MODE_SKIP_FRAME,   //!< Skip remaining frame, wait for end character
} csp_kiss_mode_t;

/**
   KISS interface data (state information).
*/
typedef struct {
	/** Max Rx length */
	unsigned int max_rx_length;
	/** Tx function */
	csp_kiss_driver_tx_t tx_func;
	/** Tx lock. Current implementation doesn't transfer data to driver in a single 'write', hence locking is necessary. */
	csp_mutex_t lock;
	/** Rx mode/state. */
	csp_kiss_mode_t rx_mode;
	/** Rx length */
	unsigned int rx_length;
	/** Rx first - if set, waiting for first character (== TNC_DATA) after start */
	bool rx_first;
	/** CSP packet for storing Rx data. */
	csp_packet_t * rx_packet;
} csp_kiss_interface_data_t;

/**
   Add interface.

   If the MTU is not set, it will be set to the csp_buffer_data_size() - sizeof(uint32_t), to make room for the CRC32 added to the packet.

   @param[in] iface CSP interface, initialized with name and inteface_data pointing to a valid #csp_kiss_interface_data_t.
   @return #CSP_ERR_NONE on success, otherwise an error code.
*/
int csp_kiss_add_interface(csp_iface_t * iface);

/**
   Send CSP packet over KISS (nexthop).

   @param[in] ifroute route.
   @param[in] packet CSP packet to send.
   @return #CSP_ERR_NONE on success, otherwise an error code.
*/
int csp_kiss_tx(const csp_route_t * ifroute, csp_packet_t * packet);

/**
   Process received CAN frame.

   Called from driver when a chunk of data has been received. Once a complete frame has been received, the CSP packet will be routed on.

   @param[in] iface incoming interface.
   @param[in] buf reveived data.
   @param[in] len length of \a buf.
   @param[out] pxTaskWoken Valid reference if called from ISR, otherwise NULL!
*/
void csp_kiss_rx(csp_iface_t * iface, const uint8_t * buf, size_t len, void * pxTaskWoken);

#ifdef __cplusplus
}
#endif
#endif
