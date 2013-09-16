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

/**
 * The KISS interface relies on the USART callback in order to parse incoming
 * messaged from the serial interface. The USART callback however does not
 * support passing the handle number of the responding USART, so you need to implement
 * a USART callback for each handle and then call kiss_rx subsequently.
 *
 * In order to initialize the KISS interface. Fist call kiss_init() and then
 * setup your usart to call csp_kiss_rx when new data is available.
 *
 * When a byte is not a part of a kiss packet, it will be returned to your
 * usart driver using the usart_insert funtion that you provide.
 *
 * @param csp_iface pointer to interface
 * @param buf pointer to incoming data
 * @param len length of incoming data
 * @param pxTaskWoken NULL if task context, pointer to variable if ISR
 */
void csp_kiss_rx(csp_iface_t * interface, uint8_t *buf, int len, void *pxTaskWoken);

/**
 * The putc function is used by the kiss interface to send
 * a string of data to the serial port. This function must
 * be implemented by the user, and passed to the kiss
 * interface through the kiss_init function.
 * @param buf byte to push
 */
typedef void (*csp_kiss_putc_f)(char buf);

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

typedef enum {
	KISS_MODE_NOT_STARTED,
	KISS_MODE_STARTED,
	KISS_MODE_ESCAPED,
	KISS_MODE_SKIP_FRAME,
} kiss_mode_e;

/**
 * This structure should be statically allocated by the user
 * and passed to the kiss interface during the init function
 * no member information should be changed
 */
typedef struct csp_kiss_handle_s {
	csp_kiss_putc_f kiss_putc;
	csp_kiss_discard_f kiss_discard;
	unsigned int rx_length;
	kiss_mode_e rx_mode;
	unsigned int rx_first;
	volatile unsigned char *rx_cbuf;
	csp_packet_t * rx_packet;
} csp_kiss_handle_t;

void csp_kiss_init(csp_iface_t * csp_iface, csp_kiss_handle_t * csp_kiss_handle, csp_kiss_putc_f kiss_putc_f, csp_kiss_discard_f kiss_discard_f, const char * name);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _CSP_IF_KISS_H_ */
