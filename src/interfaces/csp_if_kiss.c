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

#include <inttypes.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <dev/usart.h>

#include <csp/csp.h>
#include <csp/csp_endian.h>
#include <csp/csp_platform.h>
#include <csp/csp_interface.h>

#include <csp_extra/csp_if_kiss.h>

#define KISS_MODE_NOT_STARTED 0
#define KISS_MODE_STARTED 1
#define KISS_MODE_ESCAPED 2
#define KISS_MODE_ENDED 3

#define FEND  0xC0
#define FESC  0xDB
#define TFEND 0xDC
#define TFESC 0xDD

#define TNC_DATA			0x00
#define TNC_SET_HARDWARE	0x06
#define TNC_RETURN			0xFF

/** Interface definition */
csp_iface_t csp_if_kiss = {
	.name = "KISS",
	.nexthop = csp_kiss_tx,
	.mtu = 256,
};

static int usart_handle;

/* Send a CSP packet over the KISS RS232 protocol */
int csp_kiss_tx(csp_packet_t * packet, uint32_t timeout) {

	int i, txbufin;
	char txbuf[csp_if_kiss.mtu * 2];

	/* Save the outgoing id in the buffer */
	packet->id.ext = csp_hton32(packet->id.ext);
	packet->length += sizeof(packet->id.ext);

	txbuf[txbufin++] = FEND;
	txbuf[txbufin++] = TNC_DATA;
	for (i = 0; i < packet->length; i++) {
		if (((unsigned char *) &packet->id.ext)[i] == FEND) {
			((unsigned char *) &packet->id.ext)[i] = TFEND;
			txbuf[txbufin++] = FESC;
		} else if (((unsigned char *) &packet->id.ext)[i] == FESC) {
			((unsigned char *) &packet->id.ext)[i] = TFESC;
			txbuf[txbufin++] = FESC;
		}
		txbuf[txbufin++] = ((unsigned char *) &packet->id.ext)[i];

	}
	txbuf[txbufin++] = FEND;

	usart_putstr(usart_handle, txbuf, txbufin);

	csp_buffer_free(packet);

	return 1;
}

/**
 * When a frame is received, decode the kiss-stuff
 * and eventually send it directly to the CSP new packet function.
 */
void csp_kiss_rx(uint8_t * buf, int len, void * pxTaskWoken) {

	static csp_packet_t * packet;
	static int length = 0;
	static volatile unsigned char *cbuf;
	static int mode = KISS_MODE_NOT_STARTED;
	static int first = 1;

	while (len) {

		switch (mode) {
		case KISS_MODE_NOT_STARTED:
			if (*buf == FEND) {
				if (pxTaskWoken == NULL) {
					packet = csp_buffer_get(csp_if_kiss.mtu);
				} else {
					packet = csp_buffer_get_isr(csp_if_kiss.mtu);
				}
				if (packet == NULL)
					continue;
				mode = KISS_MODE_STARTED;
				cbuf = (unsigned char *) &packet->id.ext;
				first = 1;
			} else {
				/* If the char was not part of a kiss frame, send back to usart driver */
				usart_insert(usart_handle, *buf, pxTaskWoken);
			}
			break;
		case KISS_MODE_STARTED:
			if (*buf == FESC)
				mode = KISS_MODE_ESCAPED;
			else if (*buf == FEND)
				mode = KISS_MODE_ENDED;
			else {
				*cbuf = *buf;
				if (first) {
					first = 0;
					break;
				}
				length++;
				cbuf++;
			}
			break;
		case KISS_MODE_ESCAPED:
			if (*buf == TFESC) {
				*cbuf++ = FESC;
				length++;
			} else if (*buf == TFEND) {
				*cbuf++ = FEND;
				length++;
			}
			mode = KISS_MODE_STARTED;
			break;
		}

		len--;
		buf++;

		if (mode == KISS_MODE_ENDED) {

			packet->length = length;

			csp_if_kiss.frame++;

			if (packet->length >= CSP_HEADER_LENGTH && packet->length <= csp_if_kiss.mtu + CSP_HEADER_LENGTH) {
				/* Strip the CSP header off the length field before converting to CSP packet */
				packet->length -= CSP_HEADER_LENGTH;

				/* Convert the packet from network to host order */
				packet->id.ext = csp_ntoh32(packet->id.ext);

				/* Send back into CSP, notice calling from task so last argument must be NULL! */
				csp_new_packet(packet, &csp_if_kiss, pxTaskWoken);
			} else {
				csp_debug(CSP_WARN, "Weird kiss frame received! Size %u\r\n", packet->length);
				csp_buffer_free(packet);
			}

			mode = KISS_MODE_NOT_STARTED;
			length = 0;
		}
	}

}

void csp_kiss_init(int handle) {

	/* Store which handle is used */
	usart_handle = handle;

	/* Redirect USART input to csp_kiss_rs */
	usart_set_callback(usart_handle, csp_kiss_rx);

}
