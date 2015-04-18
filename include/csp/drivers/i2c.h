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

/**
 * @file i2c.h
 * Common I2C interface,
 * This file is derived from the Gomspace I2C driver,
 *
 */

#ifndef I2C_H_
#define I2C_H_

/**
 * The return value of the driver is a bit strange,
 * It should return E_NO_ERR if successfull and the value is -1
 */
#define E_NO_ERR -1

/**
 * Maximum transfer length on I2C
 */
#define I2C_MTU 	256

/**
 * I2C device modes
 */
#define I2C_MASTER 	0
#define I2C_SLAVE 	1

/**
 * Data structure for I2C frames
 */
typedef struct __attribute__((packed)) i2c_frame_s {
	uint8_t padding;
	uint8_t retries;
	uint32_t reserved;
	uint8_t dest;
	uint8_t len_rx;
	uint16_t len;
	uint8_t data[I2C_MTU];
} i2c_frame_t;

/**
 * Initialise the I2C driver
 *
 * @param handle Which I2C bus (if more than one exists)
 * @param mode I2C device mode. Must be either I2C_MASTER or I2C_SLAVE
 * @param addr Own slave address
 * @param speed Bus speed in kbps
 * @param queue_len_tx Length of transmit queue
 * @param queue_len_rx Length of receive queue
 * @param callback If this value is set, the driver will call this function instead of using an RX queue
 * @return Error code
 */
typedef void (*i2c_callback_t) (i2c_frame_t * frame, void * pxTaskWoken);
int i2c_init(int handle, int mode, uint8_t addr, uint16_t speed, int queue_len_tx, int queue_len_rx, i2c_callback_t callback);

/**
 * Send I2C frame via the selected device
 *
 * @param handle Handle to the device
 * @param frame Pointer to I2C frame
 * @param timeout Ticks to wait
 * @return Error code
 */
int i2c_send(int handle, i2c_frame_t * frame, uint16_t timeout);

#endif /* I2C_H_ */
