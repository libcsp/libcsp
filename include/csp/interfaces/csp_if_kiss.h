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

#ifndef _CSP_IF_KISS_H_
#define _CSP_IF_KISS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include <csp/csp.h>
#include <csp/csp_interface.h>

extern csp_iface_t csp_if_kiss;

/**
 * The KISS interface relies on the USART callback in order to parse incoming
 * messaged from the serial interface. This csp_kiss_rx should match the
 * usart_callback_t as implemented in your driver.
 *
 * In order to initialize the KISS interface. Fist call kiss_init() and then
 * setup your usart to call csp_kiss_rx when new data is available.
 *
 * When a byte is not a part of a kiss packet, it will be returned to your
 * usart driver using the usart_insert funtion that you provide.
 *
 * @param buf pointer to incoming data
 * @param len length of incoming data
 * @param pxTaskWoken NULL if task context, pointer to variable if ISR
 */
void csp_kiss_rx(uint8_t *buf, int len, void *pxTaskWoken);

/**
 * The putstr function is used by the kiss interface to send
 * a string of data to the serial port. This function must
 * be implemented by the user, and passed to the kiss
 * interface through the kiss_init function.
 * @param buf pointer to data
 * @param len length of data
 */
typedef void (*csp_kiss_putstr_f)(char *buf, int len);

/**
 * The characters not accepted by the kiss interface, are discarded
 * using this function, which must be implemented by the user
 * and passed through the kiss_init function.
 *
 * This reject function is typically used to display ASCII strings
 * sent over the serial port, which are not in KISS format. Such as
 * debugging information.
 *
 * @param c rejected character
 * @param pxTaskWoken NULL if task context, pointer to variable if ISR
 */
typedef void (*csp_kiss_discard_f)(char c, void *pxTaskWoken);

/**
 * Initialise kiss interface.
 * This function stores the function pointers for the putstr and discard functions,
 * which must adhere to the above function specifications. This ensures that the
 * kiss interface can use a multiple of different USART drivers.
 * @param kiss_putstr kiss uses this function to send KISS frames
 * @param kiss_discard non-kiss characters are sent to this function (set to NULL to discard)
 * @return CSP_ERR_NONE
 */
int csp_kiss_init(csp_kiss_putstr_f kiss_putstr, csp_kiss_discard_f kiss_discard);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _CSP_IF_KISS_H_ */
