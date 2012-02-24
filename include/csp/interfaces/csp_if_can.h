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

/** CAN interface modes */
#define CSP_CAN_MASKED		0
#define CSP_CAN_PROMISC		1

extern csp_iface_t csp_if_can;

/* CAN configuration struct */
struct csp_can_config {
	uint32_t bitrate;
	uint32_t clock_speed;
	char *ifc;
};

/**
 * Init CAN interface
 * @param mode Must be either CSP_CAN_MASKED or CSP_CAN_PROMISC
 * @param conf Pointer to configuration struct. 
 * @return 0 if CAN interface was successfully initialized, -1 otherwise
 */
int csp_can_init(uint8_t mode, struct csp_can_config *conf);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _CSP_IF_CAN_H_ */
