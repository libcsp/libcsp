

#include <csp/interfaces/csp_if_i2c.h>
#include <csp/csp_id.h>
#include <csp/csp.h>

int csp_i2c_tx(csp_iface_t * iface, uint16_t via, csp_packet_t * packet, int from_me) {

	/* Loopback */
	if (packet->id.dst == iface->addr) {
		csp_qfifo_write(packet, iface, NULL);
		return CSP_ERR_NONE;
	}

    /* Prepend the CSP ID to the packet */
    csp_id_prepend(packet);

	/* Use cfpid to transfer the physical destination address to the driver */
    packet->cfpid = (via != CSP_NO_VIA_ADDRESS) ? via : packet->id.dst;

    /* There is only 7 address bits available on CSP, so use the lower 7 bits for destination */
    packet->cfpid = packet->cfpid & 0x7F;

	/* send frame */
	csp_i2c_interface_data_t * ifdata = iface->interface_data;
	return (ifdata->tx_func)(iface->driver_data, packet);
}

/**
 * When a frame is received, cast it to a csp_packet
 * and send it directly to the CSP new packet function.
 * Context: ISR only
 */
void csp_i2c_rx(csp_iface_t * iface, csp_packet_t * packet, void * pxTaskWoken) {

	/* Validate input */
	if (packet == NULL) {
		return;
	}

	if (packet->frame_length < sizeof(uint32_t)) {
		iface->frame++;
		(pxTaskWoken != NULL) ? csp_buffer_free_isr(packet) : csp_buffer_free(packet);
		return;
	}

	/* We dont need to check for overflow, since this is already done at the driver level when inserting data into the buffer */

	/* Strip the CSP header off the length field before converting to CSP packet */
    csp_id_strip(packet);

	/* Receive the packet in CSP */
	csp_qfifo_write(packet, iface, pxTaskWoken);
}

int csp_i2c_add_interface(csp_iface_t * iface) {

	if ((iface == NULL) || (iface->name == NULL) || (iface->interface_data == NULL)) {
		return CSP_ERR_INVAL;
	}

	csp_i2c_interface_data_t * ifdata = iface->interface_data;
	if (ifdata->tx_func == NULL) {
		return CSP_ERR_INVAL;
	}

	iface->nexthop = csp_i2c_tx;

	csp_iflist_add(iface);

	return CSP_ERR_NONE;
}
