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

#ifndef CSP_DRIVERS_USART_H
#define CSP_DRIVERS_USART_H

/**
   @file

   USART driver.

   @note This interface implementation only support ONE open UART connection.
 */

#include <csp/csp_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
   Usart configuration.
   @see usart_open()
*/
typedef struct usart_conf {
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
} usart_conf_t;

/**
   Callback for returning data to application.

   @param[in] buf data received.
   @param[in] len data length (number of bytes in \a buf).
   @param[out] pxTaskWoken Valid reference if called from ISR, otherwise NULL!
*/
typedef void (*usart_callback_t) (void * user_data, uint8_t *buf, size_t len, void *pxTaskWoken);

/**
   Opens an UART device.

   Opens the UART device and creates a thread for reading/returning data to the application.

   @note On read failure, exit() will be called - terminating the process.

   @param[in] conf UART configuration.
   @param[in] rx_callback receive data callback.
   @param[in] user_data reference forwarded to the \a rx_callback function.
   @param[out] fd the opened file descriptor.
   @return #CSP_ERR_NONE on success, otherwise an error code.
*/
int usart_open(const usart_conf_t *conf, usart_callback_t rx_callback, void * user_data, int * fd);

/**
   Opens UART device and add KISS interface.

   This is a convience function for opening an UART devie and adding it as an interface with a given name.

   @note On read failures, exit() will be called - terminating the process.

   @param[in] conf UART configuration.
   @param[in] ifname internface name (will be copied), or use NULL for default name.
   @param[out] return_iface the added interface.
   @return #CSP_ERR_NONE on success, otherwise an error code.
*/
int usart_open_and_add_kiss_interface(const usart_conf_t *conf, const char * ifname, csp_iface_t ** return_iface);

#ifdef __cplusplus
}
#endif
#endif
