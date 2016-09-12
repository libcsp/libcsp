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

#ifndef _CAN_H_
#define _CAN_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include <csp/csp.h>
#include <csp/interfaces/csp_if_can.h>

/* The can_frame_t and can_id_t types intentionally matches the
 * can_frame struct and can_id types in include/linux/can.h
 */

typedef enum {
	CAN_ERROR = 0,
	CAN_NO_ERROR = 1,
} can_error_t;

/**
 * These functions needs to be implemented by the driver
 */
int csp_driver_can_init(uint32_t id, uint32_t mask, struct csp_can_config *conf);
int csp_driver_can_send(uint32_t id, uint8_t * data, uint8_t dlc);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _CAN_H_ */
