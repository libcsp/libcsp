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
#include <spi.h>
#include <logger/logger.h>

static int sband_tx(int fd, const void *data, size_t len) {
#if SBAND_IS_STUBBED == 0
    char buf[96];
    char *msg = buf;
    const uint8_t *ptr = data;
    for (int i=0; i<len && i<32; i++) {
        sprintf(msg, "%02x ", ptr[i]);
        msg += 3;
    }
    ex2_log(buf);
#else
    SPISbandTx(data, len);
#endif
    return 0;
}

int sdr_sband_driver_init(sdr_interface_data_t *ifdata) {
    ifdata->fd = (uintptr_t) 0;
    ifdata->tx_func = sband_tx;

    return 0;
}
