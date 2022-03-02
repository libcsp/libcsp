/*
 * Copyright (C) 2021  University of Alberta
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
/**
 * @file can.c
 * @author Andrew Rooney
 * @date 2021-02-17
 */

#ifndef LIBCSP_INCLUDE_CSP_DRIVERS_CAN_H_
#define LIBCSP_INCLUDE_CSP_DRIVERS_CAN_H_
#include <csp/interfaces/csp_if_can.h>

int csp_can_open_and_add_interface(const char * ifname, csp_iface_t ** return_iface);


#endif /* LIBCSP_INCLUDE_CSP_DRIVERS_CAN_H_ */
