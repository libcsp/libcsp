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

#ifndef CSP_INTERFACES_CSP_IF_CAN_H
#define CSP_INTERFACES_CSP_IF_CAN_H

/**
   @file

   CAN interface.

   CAN frames contains at most 8 bytes of data, so in order to transmit CSP 
   packets larger than this, a fragmentation protocol is required.
   The CAN Fragmentation Protocol (CFP) is based on CAN2.0B, using all 29 bits of the
   identifier. The CAN identifier is divided into these fields:

   - Source:       5 bits
   - Destination:  5 bits
   - Type:         1 bit
   - Remain:       8 bits
   - Identifier:   10 bits

   The \b Source and \b Destination fields must match the source and destiantion addressses in the CSP packet.
   The \b Type field is used to distinguish the first and subsequent frames in a fragmented CSP
   packet. Type is BEGIN (0) for the first fragment and MORE (1) for all other fragments.
   The \b Remain field indicates number of remaining fragments, and must be decremented by one for each fragment sent.
   The \b identifier field serves the same purpose as in the Internet Protocol, and should be an auto incrementing
   integer to uniquely separate sessions.

   Other CAN communication using a standard 11 bit identifier, can co-exist on the wire.
*/

#include <csp/csp_interface.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
   @defgroup CFP_SIZE CAN message id field size.
   @{
*/
/** Host - source/destination address. */
#define CFP_HOST_SIZE		5
/** Type - \a begin fragment or \a more fragments. */
#define CFP_TYPE_SIZE		1
/** Remaining fragments */
#define CFP_REMAIN_SIZE		8
/** CFP identification. */
#define CFP_ID_SIZE		10
/** @} */

/**
   @defgroup CFP_FIELDS Macros for extracting fields from CAN message id.
   @{
*/
/** Helper macro */
#define CFP_FIELD(id,rsiz,fsiz) ((uint32_t)((uint32_t)((id) >> (rsiz)) & (uint32_t)((1 << (fsiz)) - 1)))
/** Extract source address */
#define CFP_SRC(id)		CFP_FIELD(id, CFP_HOST_SIZE + CFP_TYPE_SIZE + CFP_REMAIN_SIZE + CFP_ID_SIZE, CFP_HOST_SIZE)
/** Extract destination address */
#define CFP_DST(id)		CFP_FIELD(id, CFP_TYPE_SIZE + CFP_REMAIN_SIZE + CFP_ID_SIZE, CFP_HOST_SIZE)
/** Extract type (begin or more) */
#define CFP_TYPE(id)		CFP_FIELD(id, CFP_REMAIN_SIZE + CFP_ID_SIZE, CFP_TYPE_SIZE)
/** Extract remaining fragments */
#define CFP_REMAIN(id)		CFP_FIELD(id, CFP_ID_SIZE, CFP_REMAIN_SIZE)
/** Extract CFP identification */
#define CFP_ID(id)		CFP_FIELD(id, 0, CFP_ID_SIZE)
/** @} */

/**
   @defgroup CFP_MAKE Macros for building CAN message id.
   @{
*/
/** Helper macro */
#define CFP_MAKE_FIELD(id,fsiz,rsiz) ((uint32_t)(((id) & (uint32_t)((uint32_t)(1 << (fsiz)) - 1)) << (rsiz)))
/** Make source */
#define CFP_MAKE_SRC(id)	CFP_MAKE_FIELD(id, CFP_HOST_SIZE, CFP_HOST_SIZE + CFP_TYPE_SIZE + CFP_REMAIN_SIZE + CFP_ID_SIZE)
/** Make destination */
#define CFP_MAKE_DST(id)	CFP_MAKE_FIELD(id, CFP_HOST_SIZE, CFP_TYPE_SIZE + CFP_REMAIN_SIZE + CFP_ID_SIZE)
/** Make type */
#define CFP_MAKE_TYPE(id)	CFP_MAKE_FIELD(id, CFP_TYPE_SIZE, CFP_REMAIN_SIZE + CFP_ID_SIZE)
/** Make remaining fragments */
#define CFP_MAKE_REMAIN(id)	CFP_MAKE_FIELD(id, CFP_REMAIN_SIZE, CFP_ID_SIZE)
/** Make CFP id */
#define CFP_MAKE_ID(id)		CFP_MAKE_FIELD(id, CFP_ID_SIZE, 0)
/** @} */

/** Mask to uniquely separate connections */
#define CFP_ID_CONN_MASK	(CFP_MAKE_SRC((uint32_t)(1 << CFP_HOST_SIZE) - 1) | \
				 CFP_MAKE_DST((uint32_t)(1 << CFP_HOST_SIZE) - 1) | \
				 CFP_MAKE_ID((uint32_t)(1 << CFP_ID_SIZE) - 1))

/**
   Default interface name.
*/
#define CSP_IF_CAN_DEFAULT_NAME "CAN"

/**
   Send CAN frame (implemented by driver).

   Used by csp_can_tx() to send CAN frames.

   @param[in] driver_data driver data from #csp_iface_t
   @param[in] id CAM message id.
   @param[in] data CAN data 
   @param[in] dlc data length of \a data.
   @return #CSP_ERR_NONE on success, otherwise an error code.
*/
typedef int (*csp_can_driver_tx_t)(void * driver_data, uint32_t id, const uint8_t * data, uint8_t dlc);

/**
   Interface data (state information).
*/
typedef struct {
    /** CFP Identification number - same number on all fragments from same CSP packet. */
    uint32_t cfp_frame_id;
    /** Tx function */
    csp_can_driver_tx_t tx_func;
} csp_can_interface_data_t;

/**
   Add interface.

   If the MTU is not set, it will be set to the maximum value of 2042 bytes (max length when using CFP).

   @param[in] iface CSP interface, initialized with name and inteface_data pointing to a valid #csp_can_interface_data_t structure.
   @return #CSP_ERR_NONE on success, otherwise an error code.
*/
int csp_can_add_interface(csp_iface_t * iface);

/**
   Send CSP packet over CAN (nexthop).

   This function will split the CSP packet into several fragments and call csp_can_tx_fram() for sending each fragment.

   @param[in] ifroute route.
   @param[in] packet CSP packet to send.
   @return #CSP_ERR_NONE on success, otherwise an error code.
*/
int csp_can_tx(const csp_route_t * ifroute, csp_packet_t *packet);

/**
   Process received CAN frame.

   Called from driver when a single CAN frame (up to 8 bytes) has been received. The function will gather the fragments into a single
   CSP packet and route it on when complete.

   @param[in] iface incoming interface.
   @param[in] id received CAN message identifier.
   @param[in] data received CAN data.
   @param[in] dlc length of received \a data.
   @param[out] pxTaskWoken Valid reference if called from ISR, otherwise NULL!
   @return #CSP_ERR_NONE on success, otherwise an error code.
*/
int csp_can_rx(csp_iface_t * iface, uint32_t id, const uint8_t * data, uint8_t dlc, CSP_BASE_TYPE *pxTaskWoken);

#ifdef __cplusplus
}
#endif
#endif
