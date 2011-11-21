/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2011 GomSpace ApS (http://www.gomspace.com)
Copyright (C) 2011 AAUSAT3 Project (http://aausat3.space.aau.dk)

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
 * @file usart.h
 * Common USART interface
 *
 * @author Johan De Claville Christiansen
 * Copyright 2011 GomSpace ApS. All rights reserved.
 */

#ifndef USART_H_
#define USART_H_

#include <stdint.h>

/**
 * Initialise UART
 *
 * @param handle usart[0,1,2,3]
 * @param fcpu CPU speed in Hz
 * @param usart_baud Baud rate in bps
 */
void usart_init(int handle, uint32_t fcpu, uint32_t usart_baud);

/**
 * In order to catch incoming chars use the callback.
 * Only one callback per interface.
 * @param handle usart[0,1,2,3]
 * @param callback function pointer
 */
typedef void (*usart_callback_t) (uint8_t * buf, int len, void * pxTaskWoken);
void usart_set_callback(int handle, usart_callback_t callback);

/**
 * Insert a character to the RX buffer of a usart
 * @param handle usart[0,1,2,3]
 * @param c Character to insert
 */
void usart_insert(int handle, char c, void * pxTaskWoken);

/**
 * Polling putchar
 *
 * @param handle usart[0,1,2,3]
 * @param c Character to transmit
 */
void usart_putc(int handle, char c);

/**
 * Send char buffer on UART
 *
 * @param handle usart[0,1,2,3]
 * @param buf Pointer to data
 * @param len Length of data
 */
void usart_putstr(int handle, char *buf, int len);

/**
 * Buffered getchar
 *
 * @param handle usart[0,1,2,3]
 * @return Character received
 */
char usart_getc(int handle);

/**
 * Buffered getchar (not blocking)
 *
 * @param handle usart[0,1,2,3]
 * @return Character received
 */
char usart_getc_nblock(int handle);

#endif /* USART_H_ */
