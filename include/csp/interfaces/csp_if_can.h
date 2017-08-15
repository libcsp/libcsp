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

#ifndef _CSP_IF_CAN_H_
#define _CSP_IF_CAN_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include <csp/csp.h>
#include <csp/csp_interface.h>

/* CAN header macros */
#define CFP_HOST_SIZE       5
#define CFP_TYPE_SIZE       1
#define CFP_REMAIN_SIZE     8
#define CFP_ID_SIZE         10

/* Macros for extracting header fields */
#define CFP_FIELD(id,rsiz,fsiz) ((uint32_t)((uint32_t)((id) >> (rsiz)) & (uint32_t)((1 << (fsiz)) - 1)))
#define CFP_SRC(id)			CFP_FIELD(id, CFP_HOST_SIZE + CFP_TYPE_SIZE + CFP_REMAIN_SIZE + CFP_ID_SIZE, CFP_HOST_SIZE)
#define CFP_DST(id)			CFP_FIELD(id, CFP_TYPE_SIZE + CFP_REMAIN_SIZE + CFP_ID_SIZE, CFP_HOST_SIZE)
#define CFP_TYPE(id)		CFP_FIELD(id, CFP_REMAIN_SIZE + CFP_ID_SIZE, CFP_TYPE_SIZE)
#define CFP_REMAIN(id)		CFP_FIELD(id, CFP_ID_SIZE, CFP_REMAIN_SIZE)
#define CFP_ID(id)			CFP_FIELD(id, 0, CFP_ID_SIZE)

/* Macros for building CFP headers */
#define CFP_MAKE_FIELD(id,fsiz,rsiz) ((uint32_t)(((id) & (uint32_t)((uint32_t)(1 << (fsiz)) - 1)) << (rsiz)))
#define CFP_MAKE_SRC(id)	CFP_MAKE_FIELD(id, CFP_HOST_SIZE, CFP_HOST_SIZE + CFP_TYPE_SIZE + CFP_REMAIN_SIZE + CFP_ID_SIZE)
#define CFP_MAKE_DST(id)	CFP_MAKE_FIELD(id, CFP_HOST_SIZE, CFP_TYPE_SIZE + CFP_REMAIN_SIZE + CFP_ID_SIZE)
#define CFP_MAKE_TYPE(id)	CFP_MAKE_FIELD(id, CFP_TYPE_SIZE, CFP_REMAIN_SIZE + CFP_ID_SIZE)
#define CFP_MAKE_REMAIN(id)	CFP_MAKE_FIELD(id, CFP_REMAIN_SIZE, CFP_ID_SIZE)
#define CFP_MAKE_ID(id)		CFP_MAKE_FIELD(id, CFP_ID_SIZE, 0)

/* Mask to uniquely separate connections */
#define CFP_ID_CONN_MASK	(CFP_MAKE_SRC((uint32_t)(1 << CFP_HOST_SIZE) - 1) | \
				 CFP_MAKE_DST((uint32_t)(1 << CFP_HOST_SIZE) - 1) | \
				 CFP_MAKE_ID((uint32_t)(1 << CFP_ID_SIZE) - 1))

/* Maximum Transmission Unit for CSP over CAN */
#define CSP_CAN_MTU	256

int csp_can_rx(csp_iface_t *interface, uint32_t id, const uint8_t * data, uint8_t dlc, CSP_BASE_TYPE *task_woken);
int csp_can_tx(csp_iface_t *interface, csp_packet_t *packet, uint32_t timeout);

/* Must be implemented by the driver */
int csp_can_tx_frame(csp_iface_t *interface, uint32_t id, const uint8_t * data, uint8_t dlc);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _CSP_IF_CAN_H_ */
