

/**
 * Testing:
 * Use the following linux tool to setup loopback port to test with:
 * socat -d -d pty,raw,echo=0 pty,raw,echo=0
 */

#include <csp/interfaces/csp_if_kiss.h>
#include <csp/drivers/usart.h>
#include <string.h>

#include <endian.h>
#include <csp/csp_crc32.h>
#include <csp/csp_id.h>

#define FEND     0xC0
#define FESC     0xDB
#define TFEND    0xDC
#define TFESC    0xDD
#define TNC_DATA 0x00

int csp_kiss_tx(csp_iface_t * iface, uint16_t via, csp_packet_t * packet, int from_me) {

	csp_kiss_interface_data_t * ifdata = iface->interface_data;
	void * driver = iface->driver_data;

	/* Lock (before modifying packet) */
	csp_usart_lock(driver);

	/* Add CRC32 checksum */
	csp_crc32_append(packet);

	/* Save the outgoing id in the buffer */
	csp_id_prepend(packet);

	/* Transmit data */
	const unsigned char start[] = {FEND, TNC_DATA};
	const unsigned char esc_end[] = {FESC, TFEND};
	const unsigned char esc_esc[] = {FESC, TFESC};
	const unsigned char * data = packet->frame_begin;

	ifdata->tx_func(driver, start, sizeof(start));

	for (unsigned int i = 0; i < packet->frame_length; i++, ++data) {
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

	/* Unlock */
	csp_usart_unlock(driver);

	/* Free data */
	csp_buffer_free(packet);

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

		/* If packet was too long, truncate and restart */
		if (ifdata->rx_packet != NULL && &ifdata->rx_packet->frame_begin[ifdata->rx_length] >= &ifdata->rx_packet->data[sizeof(ifdata->rx_packet->data)]) {
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

				/* Always allocate new buffer */
				if (ifdata->rx_packet == NULL) {
					ifdata->rx_packet = pxTaskWoken ? csp_buffer_get_always_isr() : csp_buffer_get_always();
				}

				/* If no more memory, skip frame */
				if (ifdata->rx_packet == NULL) {
					ifdata->rx_mode = KISS_MODE_SKIP_FRAME;
					iface->drop++;
					break;
				}

				/* Start transfer */
				csp_id_setup_rx(ifdata->rx_packet);
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

						ifdata->rx_packet->frame_length = ifdata->rx_length;
						if (csp_id_strip(ifdata->rx_packet) < 0) {
							iface->frame++;
							ifdata->rx_mode = KISS_MODE_NOT_STARTED;
							break;
						}

						/* Validate CRC */
						if (csp_crc32_verify(ifdata->rx_packet) != CSP_ERR_NONE) {
							iface->frame++;
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
				ifdata->rx_packet->frame_begin[ifdata->rx_length++] = inputbyte;

				break;

			case KISS_MODE_ESCAPED:

				/* Escaped escape char */
				if (inputbyte == TFESC)
					ifdata->rx_packet->frame_begin[ifdata->rx_length++] = FESC;

				/* Escaped fend char */
				if (inputbyte == TFEND)
					ifdata->rx_packet->frame_begin[ifdata->rx_length++] = FEND;

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

	ifdata->rx_length = 0;
	ifdata->rx_mode = KISS_MODE_NOT_STARTED;
	ifdata->rx_first = false;
	ifdata->rx_packet = NULL;

	iface->nexthop = csp_kiss_tx;

	csp_iflist_add(iface);

	return CSP_ERR_NONE;
}
