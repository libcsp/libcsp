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

#include <inttypes.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <csp/csp.h>
#include <csp/csp_endian.h>
#include <csp/csp_platform.h>
#include <csp/csp_interface.h>
#include <csp/interfaces/csp_if_kiss.h>
#include <csp/arch/csp_semaphore.h>
#include <csp/csp_crc32.h>

#define KISS_CRC32 				1
#define KISS_MTU				256

#define KISS_MODE_NOT_STARTED 	0
#define KISS_MODE_STARTED 		1
#define KISS_MODE_ESCAPED 		2
#define KISS_MODE_ENDED 		3

#define FEND  					0xC0
#define FESC  					0xDB
#define TFEND 					0xDC
#define TFESC 					0xDD

#define TNC_DATA				0x00
#define TNC_SET_HARDWARE		0x06
#define TNC_RETURN				0xFF

static int kiss_lock_init = 0;
static csp_bin_sem_handle_t kiss_lock;

/* Send a CSP packet over the KISS RS232 protocol */
static int csp_kiss_tx(csp_iface_t * interface, csp_packet_t * packet, uint32_t timeout) {

	if (interface == NULL || interface->driver == NULL)
		return CSP_ERR_DRIVER;

	/* Add CRC32 checksum */
#if defined(KISS_CRC32)
	csp_crc32_append(packet);
#endif

	/* Save the outgoing id in the buffer */
	packet->id.ext = csp_hton32(packet->id.ext);
	packet->length += sizeof(packet->id.ext);

	/* Lock */
	csp_bin_sem_wait(&kiss_lock, 1000);

	/* Transmit data */
	csp_kiss_handle_t * driver = interface->driver;
	driver->kiss_putc(FEND);
	driver->kiss_putc(TNC_DATA);
	for (unsigned int i = 0; i < packet->length; i++) {
		if (((unsigned char *) &packet->id.ext)[i] == FEND) {
			((unsigned char *) &packet->id.ext)[i] = TFEND;
			driver->kiss_putc(FESC);
		} else if (((unsigned char *) &packet->id.ext)[i] == FESC) {
			((unsigned char *) &packet->id.ext)[i] = TFESC;
			driver->kiss_putc(FESC);
		}
		driver->kiss_putc(((unsigned char *) &packet->id.ext)[i]);
	}
	driver->kiss_putc(FEND);

	/* Free data */
	csp_buffer_free(packet);

	/* Unlock */
	csp_bin_sem_post(&kiss_lock);

	return CSP_ERR_NONE;
}

/**
 * When a frame is received, decode the kiss-stuff
 * and eventually send it directly to the CSP new packet function.
 */
void csp_kiss_rx(csp_iface_t * interface, uint8_t * buf, int len, void * pxTaskWoken) {

	static csp_packet_t * packet = NULL;
	static int length = 0;
	static volatile unsigned char *cbuf;
	static int mode = KISS_MODE_NOT_STARTED;
	static int first = 1;

	csp_kiss_handle_t * driver = interface->driver;

	while (len) {

		switch (mode) {
		case KISS_MODE_NOT_STARTED:
			if (*buf == FEND) {
				if (packet == NULL) {
					if (pxTaskWoken == NULL) {
						packet = csp_buffer_get(interface->mtu);
					} else {
						packet = csp_buffer_get_isr(interface->mtu);
					}
				}

				if (packet != NULL) {
					cbuf = (unsigned char *) &packet->id.ext;
				} else {
					cbuf = NULL;
				}

				mode = KISS_MODE_STARTED;
				first = 1;
			} else {
				/* If the char was not part of a kiss frame, send back to usart driver */
				if (driver->kiss_discard != NULL)
					driver->kiss_discard(*buf, pxTaskWoken);
			}
			break;
		case KISS_MODE_STARTED:
			if (*buf == FESC)
				mode = KISS_MODE_ESCAPED;
			else if (*buf == FEND) {
				if (length > 0) {
					mode = KISS_MODE_ENDED;
				}
			} else {
				if (cbuf != NULL)
					*cbuf = *buf;
				if (first) {
					first = 0;
					break;
				}
				if (cbuf != NULL)
					cbuf++;
				length++;
			}
			break;
		case KISS_MODE_ESCAPED:
			if (*buf == TFESC) {
				if (cbuf != NULL)
					*cbuf++ = FESC;
				length++;
			} else if (*buf == TFEND) {
				if (cbuf != NULL)
					*cbuf++ = FEND;
				length++;
			}
			mode = KISS_MODE_STARTED;
			break;
		}

		len--;
		buf++;

		if (length >= 256) {
			interface->rx_error++;
			mode = KISS_MODE_NOT_STARTED;
			length = 0;
			csp_log_warn("KISS RX overflow\r\n");
			continue;
		}

		if (mode == KISS_MODE_ENDED) {

			if (packet == NULL) {
				mode = KISS_MODE_NOT_STARTED;
				length = 0;
				continue;
			}

			packet->length = length;

			interface->frame++;

			if (packet->length >= CSP_HEADER_LENGTH && packet->length <= interface->mtu + CSP_HEADER_LENGTH) {

				/* Strip the CSP header off the length field before converting to CSP packet */
				packet->length -= CSP_HEADER_LENGTH;

				/* Convert the packet from network to host order */
				packet->id.ext = csp_ntoh32(packet->id.ext);

#if defined(KISS_CRC32)
				if (csp_crc32_verify(packet) != CSP_ERR_NONE) {
					csp_log_warn("KISS invalid CRC len %u\r\n", packet->length);
					interface->rx_error++;
					mode = KISS_MODE_NOT_STARTED;
					length = 0;
					continue;
				}
#endif

				/* Send back into CSP, notice calling from task so last argument must be NULL! */
				csp_new_packet(packet, interface, pxTaskWoken);
				packet = NULL;
			} else {
				csp_log_warn("Weird kiss frame received! Size %u\r\n", packet->length);
			}

			mode = KISS_MODE_NOT_STARTED;
			length = 0;

		}

	}

}

void csp_kiss_init(csp_iface_t * csp_iface, csp_kiss_handle_t * csp_kiss_handle, csp_kiss_putc_f kiss_putc_f, csp_kiss_discard_f kiss_discard_f, const char * name) {

	/* Init lock only once */
	if (kiss_lock_init == 0) {
		csp_bin_sem_create(&kiss_lock);
		kiss_lock_init = 1;
	}

	/* Register device handle as member of interface */
	csp_iface->driver = csp_kiss_handle;
	csp_kiss_handle->kiss_discard = kiss_discard_f;
	csp_kiss_handle->kiss_putc = kiss_putc_f;

	/* Setop other mandatories */
	csp_iface->mtu = KISS_MTU;
	csp_iface->nexthop = csp_kiss_tx;
	csp_iface->name = name;

	/* Regsiter interface */
	csp_route_add_if(csp_iface);

}
