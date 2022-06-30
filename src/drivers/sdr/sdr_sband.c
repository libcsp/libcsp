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
 * @file sdr_sband.c
 * @author Ron Unrau
 * @date 2022-07-01
 */

#include <string.h>
#include <csp/csp.h>
#include <sdr_driver.h>

int sdr_sband_driver_init(sdr_interface_data_t *ifdata) {
    ifdata->fd = (uintptr_t) 0;
    ifdata->tx_func = (sdr_tx_t) 0;

    return CSP_ERR_NONE;
}
