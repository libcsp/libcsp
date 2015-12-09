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

#define KISS_MTU				256

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
	csp_crc32_append(packet, false);

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

	/* Driver handle */
	csp_kiss_handle_t * driver = interface->driver;

	while (len--) {

		/* Input */
		unsigned char inputbyte = *buf++;

		/* If packet was too long */
		if (driver->rx_length > interface->mtu) {
			csp_log_warn("KISS RX overflow");
			interface->rx_error++;
			driver->rx_mode = KISS_MODE_NOT_STARTED;
			driver->rx_length = 0;
		}

		switch (driver->rx_mode) {

		case KISS_MODE_NOT_STARTED:

			/* Send normal chars back to usart driver */
			if (inputbyte != FEND) {
				if (driver->kiss_discard != NULL)
					driver->kiss_discard(inputbyte, pxTaskWoken);
				break;
			}

			/* Try to allocate new buffer */
			if (driver->rx_packet == NULL) {
				if (pxTaskWoken == NULL) {
					driver->rx_packet = csp_buffer_get(interface->mtu);
				} else {
					driver->rx_packet = csp_buffer_get_isr(interface->mtu);
				}
			}

			/* If no more memory, skip frame */
			if (driver->rx_packet == NULL) {
				driver->rx_mode = KISS_MODE_SKIP_FRAME;
				break;
			}

			/* Start transfer */
			driver->rx_length = 0;
			driver->rx_mode = KISS_MODE_STARTED;
			driver->rx_first = 1;
			break;

		case KISS_MODE_STARTED:

			/* Escape char */
			if (inputbyte == FESC) {
				driver->rx_mode = KISS_MODE_ESCAPED;
				break;
			}

			/* End Char */
			if (inputbyte == FEND) {

				/* Accept message */
				if (driver->rx_length > 0) {

					/* Check for valid length */
					if (driver->rx_length < CSP_HEADER_LENGTH + sizeof(uint32_t)) {
						csp_log_warn("KISS short frame skipped, len: %u", driver->rx_length);
						interface->rx_error++;
						driver->rx_mode = KISS_MODE_NOT_STARTED;
						break;
					}

					/* Count received frame */
					interface->frame++;

					/* The CSP packet length is without the header */
					driver->rx_packet->length = driver->rx_length - CSP_HEADER_LENGTH;

					/* Convert the packet from network to host order */
					driver->rx_packet->id.ext = csp_ntoh32(driver->rx_packet->id.ext);

					/* Validate CRC */
					if (csp_crc32_verify(driver->rx_packet, false) != CSP_ERR_NONE) {
						csp_log_warn("KISS invalid crc frame skipped, len: %u", driver->rx_packet->length);
						interface->rx_error++;
						driver->rx_mode = KISS_MODE_NOT_STARTED;
						break;
					}

					/* Send back into CSP, notice calling from task so last argument must be NULL! */
					csp_qfifo_write(driver->rx_packet, interface, pxTaskWoken);
					driver->rx_packet = NULL;
					driver->rx_mode = KISS_MODE_NOT_STARTED;
					break;

				}

				/* Break after the end char */
				break;
			}

			/* Skip the first char after FEND which is TNC_DATA (0x00) */
			if (driver->rx_first) {
				driver->rx_first = 0;
				break;
			}

			/* Valid data char */
			((char *) &driver->rx_packet->id.ext)[driver->rx_length++] = inputbyte;

			break;

		case KISS_MODE_ESCAPED:

			/* Escaped escape char */
			if (inputbyte == TFESC)
				((char *) &driver->rx_packet->id.ext)[driver->rx_length++] = FESC;

			/* Escaped fend char */
			if (inputbyte == TFEND)
				((char *) &driver->rx_packet->id.ext)[driver->rx_length++] = FEND;

			/* Go back to started mode */
			driver->rx_mode = KISS_MODE_STARTED;
			break;

		case KISS_MODE_SKIP_FRAME:

			/* Just wait for end char */
			if (inputbyte == FEND)
				driver->rx_mode = KISS_MODE_NOT_STARTED;

			break;

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
	csp_kiss_handle->rx_packet = NULL;
	csp_kiss_handle->rx_mode = KISS_MODE_NOT_STARTED;

	/* Setop other mandatories */
	csp_iface->mtu = KISS_MTU;
	csp_iface->nexthop = csp_kiss_tx;
	csp_iface->name = name;

	/* Regsiter interface */
	csp_iflist_add(csp_iface);

}
