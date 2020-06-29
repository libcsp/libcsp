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

#include <csp/interfaces/csp_if_kiss.h>

#include <string.h>

#include <csp/csp_endian.h>
#include <csp/csp_crc32.h>

#define FEND  		0xC0
#define FESC  		0xDB
#define TFEND 		0xDC
#define TFESC 		0xDD
#define TNC_DATA	0x00

int csp_kiss_tx(const csp_route_t * ifroute, csp_packet_t * packet) {

	csp_kiss_interface_data_t * ifdata = ifroute->iface->interface_data;
	void * driver = ifroute->iface->driver_data;

	/* Add CRC32 checksum - the MTU setting ensures there are space */
	csp_crc32_append(packet, false);

	/* Lock */
	if (csp_mutex_lock(&ifdata->lock, 1000) != CSP_MUTEX_OK) {
            return CSP_ERR_TIMEDOUT;
        }

	/* Save the outgoing id in the buffer */
	packet->id.ext = csp_hton32(packet->id.ext);
	packet->length += sizeof(packet->id.ext);

	/* Transmit data */
        const unsigned char start[] = {FEND, TNC_DATA};
        const unsigned char esc_end[] = {FESC, TFEND};
        const unsigned char esc_esc[] = {FESC, TFESC};
        const unsigned char * data = (unsigned char *) &packet->id.ext;
        ifdata->tx_func(driver, start, sizeof(start));
	for (unsigned int i = 0; i < packet->length; i++, ++data) {
		if (*data == FEND) {
                    ifdata->tx_func(driver, esc_end, sizeof(esc_end));
                    continue;
		}
                if (*data == FESC) {
                    ifdata->tx_func(driver, esc_esc, sizeof(esc_esc));
                    continue;
		}
		ifdata->tx_func(driver, data, 1);
	}
        const unsigned char stop[] = {FEND};
        ifdata->tx_func(driver, stop, sizeof(stop));

	/* Free data */
	csp_buffer_free(packet);

	/* Unlock */
	csp_mutex_unlock(&ifdata->lock);

	return CSP_ERR_NONE;
}

/**
 * Decode received data and eventually route the packet.
 */
void csp_kiss_rx(csp_iface_t * iface, const uint8_t * buf, size_t len, void * pxTaskWoken) {

	csp_kiss_interface_data_t * ifdata = iface->interface_data;

	while (len--) {

		/* Input */
		uint8_t inputbyte = *buf++;

		/* If packet was too long */
		if (ifdata->rx_length > ifdata->max_rx_length) {
			//csp_log_warn("KISS RX overflow");
			iface->rx_error++;
			ifdata->rx_mode = KISS_MODE_NOT_STARTED;
			ifdata->rx_length = 0;
		}

		switch (ifdata->rx_mode) {

		case KISS_MODE_NOT_STARTED:

			/* Skip any characters until End char detected */
			if (inputbyte != FEND) {
				break;
			}

			/* Try to allocate new buffer */
			if (ifdata->rx_packet == NULL) {
				ifdata->rx_packet = pxTaskWoken ? csp_buffer_get_isr(0) : csp_buffer_get(0); // CSP only supports one size
			}

			/* If no more memory, skip frame */
			if (ifdata->rx_packet == NULL) {
				ifdata->rx_mode = KISS_MODE_SKIP_FRAME;
				break;
			}

			/* Start transfer */
			ifdata->rx_length = 0;
			ifdata->rx_mode = KISS_MODE_STARTED;
			ifdata->rx_first = true;
			break;

		case KISS_MODE_STARTED:

			/* Escape char */
			if (inputbyte == FESC) {
				ifdata->rx_mode = KISS_MODE_ESCAPED;
				break;
			}

			/* End Char */
			if (inputbyte == FEND) {

				/* Accept message */
				if (ifdata->rx_length > 0) {

					/* Check for valid length */
					if (ifdata->rx_length < CSP_HEADER_LENGTH + sizeof(uint32_t)) {
						//csp_log_warn("KISS short frame skipped, len: %u", ifdata->rx_length);
						iface->rx_error++;
						ifdata->rx_mode = KISS_MODE_NOT_STARTED;
						break;
					}

					/* Count received frame */
					iface->frame++;

					/* The CSP packet length is without the header */
					ifdata->rx_packet->length = ifdata->rx_length - CSP_HEADER_LENGTH;

					/* Convert the packet from network to host order */
					ifdata->rx_packet->id.ext = csp_ntoh32(ifdata->rx_packet->id.ext);

					/* Validate CRC */
					if (csp_crc32_verify(ifdata->rx_packet, false) != CSP_ERR_NONE) {
						//csp_log_warn("KISS invalid crc frame skipped, len: %u", ifdata->rx_packet->length);
						iface->rx_error++;
						ifdata->rx_mode = KISS_MODE_NOT_STARTED;
						break;
					}

					/* Send back into CSP, notice calling from task so last argument must be NULL! */
					csp_qfifo_write(ifdata->rx_packet, iface, pxTaskWoken);
					ifdata->rx_packet = NULL;
					ifdata->rx_mode = KISS_MODE_NOT_STARTED;
					break;

				}

				/* Break after the end char */
				break;
			}

			/* Skip the first char after FEND which is TNC_DATA (0x00) */
			if (ifdata->rx_first) {
				ifdata->rx_first = false;
				break;
			}

			/* Valid data char */
			((char *) &ifdata->rx_packet->id.ext)[ifdata->rx_length++] = inputbyte;

			break;

		case KISS_MODE_ESCAPED:

			/* Escaped escape char */
			if (inputbyte == TFESC)
				((char *) &ifdata->rx_packet->id.ext)[ifdata->rx_length++] = FESC;

			/* Escaped fend char */
			if (inputbyte == TFEND)
				((char *) &ifdata->rx_packet->id.ext)[ifdata->rx_length++] = FEND;

			/* Go back to started mode */
			ifdata->rx_mode = KISS_MODE_STARTED;
			break;

		case KISS_MODE_SKIP_FRAME:

			/* Just wait for end char */
			if (inputbyte == FEND)
				ifdata->rx_mode = KISS_MODE_NOT_STARTED;

			break;

		}

	}

}

int csp_kiss_add_interface(csp_iface_t * iface) {

	if ((iface == NULL) || (iface->name == NULL) || (iface->interface_data == NULL)) {
		return CSP_ERR_INVAL;
	}

        csp_kiss_interface_data_t * ifdata = iface->interface_data;
	if (ifdata->tx_func == NULL) {
		return CSP_ERR_INVAL;
	}

	if (csp_mutex_create(&ifdata->lock) != CSP_MUTEX_OK) {
		return CSP_ERR_NOMEM;
        }

	ifdata->max_rx_length = CSP_HEADER_LENGTH + csp_buffer_data_size(); // CSP header + CSP data
	ifdata->rx_length = 0;
	ifdata->rx_mode = KISS_MODE_NOT_STARTED;
	ifdata->rx_first = false;
	ifdata->rx_packet = NULL;

        const unsigned int max_data_size = csp_buffer_data_size() - sizeof(uint32_t); // compensate for the added CRC32
        if ((iface->mtu == 0) || (iface->mtu > max_data_size)) {
            iface->mtu = max_data_size;
        }

	iface->nexthop = csp_kiss_tx;

	return csp_iflist_add(iface);
}
