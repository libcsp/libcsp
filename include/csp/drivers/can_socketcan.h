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
#ifndef LIB_CSP_INCLUDE_CSP_DRIVERS_CAN_SOCKETCAN_H_
#define LIB_CSP_INCLUDE_CSP_DRIVERS_CAN_SOCKETCAN_H_

/**
   @file

   Socket CAN driver (Linux).

   This driver requires the libsocketcan library.
*/

#include <csp/interfaces/csp_if_can.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
   Open CAN socket and add CSP interface.

   @param[in] device CAN device name (Linux device).
   @param[in] ifname CSP interface name, use #CSP_IF_CAN_DEFAULT_NAME for default name.
   @param[in] bitrate if different from 0, it will be attempted to change the bitrate on the CAN device - this may require increased OS privileges.
   @param[in] promisc if \a true, receive all CAN frames. If \a false a filter is set on the CAN device, using csp_get_address().
   @param[out] return_iface the added interface.
   @return The added interface, or NULL in case of failure.
*/
int csp_can_socketcan_open_and_add_interface(const char * device, const char * ifname, int bitrate, bool promisc, csp_iface_t ** return_iface);

/**
   Initialize socketcan and add CSP interface.

   @deprecated version 1.6, use csp_can_socketcan_open_and_add_interface()
   @param[in] device CAN device name (Linux device).
   @param[in] bitrate if different from 0, it will be attempted to change the bitrate on the CAN device - this may require increased OS privileges.
   @param[in] promisc if \a true, receive all CAN frames. If \a false a filter is set on the CAN device, using csp_get_address().
   @return The added interface, or NULL in case of failure.
*/
csp_iface_t * csp_can_socketcan_init(const char * device, int bitrate, bool promisc);

/**
   Stop the Rx thread and free resources (testing).

   @note This will invalidate CSP, because an interface can't be removed. This is primarily for testing.

   @param[in] iface interface to stop.
   @return #CSP_ERR_NONE on success, otherwise an error code.
*/
int csp_can_socketcan_stop(csp_iface_t * iface);

#ifdef __cplusplus
}
#endif
#endif
