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
 * @file
 * Common USART interface,
 * This file is derived from the Gomspace USART driver,
 * the main difference is the assumption that only one USART will be present on a PC
 */

#ifndef USART_H_
#define USART_H_

#include <stdint.h>

/**
   Usart configuration, to be used with the usart_init call.
*/
struct usart_conf {
    //! USART device.
    const char *device;
    //! bits per second.
    uint32_t baudrate;
    //! Number of data bits.
    uint8_t databits;
    //! Number of stop bits.
    uint8_t stopbits;
    //! Parity setting.
    uint8_t paritysetting;
    //! Enable parity checking (Windows only).
    uint8_t checkparity;
};

/**
   Initialise UART with the usart_conf data structure
   @param[in] conf full configuration structure
*/
void usart_init(struct usart_conf *conf);

/**
   In order to catch incoming chars use the callback.
   Only one callback per interface.
   @param[in] handle usart[0,1,2,3]
   @param[in] callback function pointer
*/
typedef void (*usart_callback_t) (uint8_t *buf, int len, void *pxTaskWoken);

/**
   Set callback for receiving data.
*/
void usart_set_callback(usart_callback_t callback);

/**
   Insert a character to the RX buffer of a usart

   @param[in] c character to insert
   @param[out] pxTaskWoken can be set, if context switch is required due to received data.
*/
void usart_insert(char c, void *pxTaskWoken);

/**
   Polling putchar (stdin).

   @param[in] c Character to transmit
*/
void usart_putc(char c);

/**
   Send char buffer on UART (stdout).

   @param[in] buf Pointer to data
   @param[in] len Length of data
*/
void usart_putstr(char *buf, int len);

/**
   Buffered getchar (stdin).

   @return Character received
*/
char usart_getc(void);

#endif /* USART_H_ */
