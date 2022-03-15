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

#include <csp/interfaces/csp_if_i2c.h>

#include <csp/csp.h>
#include <csp/csp_endian.h>

// Ensure certain fields in the csp_i2c_frame_t matches the fields in the csp_packet_t
CSP_STATIC_ASSERT(offsetof(csp_i2c_frame_t, len) == offsetof(csp_packet_t, length), len_field_misaligned);
CSP_STATIC_ASSERT(offsetof(csp_i2c_frame_t, data) == offsetof(csp_packet_t, id), data_field_misaligned);

int csp_i2c_tx(const csp_route_t * ifroute, csp_packet_t * packet) {

	/* Cast the CSP packet buffer into an i2c frame */
	csp_i2c_frame_t * frame = (csp_i2c_frame_t *) packet;

	/* Insert destination node into the i2c destination field */
	frame->dest = (ifroute->via != CSP_NO_VIA_ADDRESS) ? ifroute->via : packet->id.dst;

	/* Save the outgoing id in the buffer */
	packet->id.ext = csp_hton32(packet->id.ext);

	/* Add the CSP header to the I2C length field */
	frame->len += sizeof(packet->id);
	frame->len_rx = 0;

	/* Some I2C drivers support X number of retries
	 * CSP don't care about this. If it doesn't work the first
	 * time, don't use time on it.
	 */
	frame->retries = 0;

	/* send frame */
        csp_i2c_interface_data_t * ifdata = ifroute->iface->interface_data;
	return (ifdata->tx_func)(ifroute->iface->driver_data, frame);

}

/**
 * When a frame is received, cast it to a csp_packet
 * and send it directly to the CSP new packet function.
 * Context: ISR only
 */
void csp_i2c_rx(csp_iface_t * iface, csp_i2c_frame_t * frame, void * pxTaskWoken) {

	/* Validate input */
	if (frame == NULL) {
		return;
	}

	if (frame->len < sizeof(csp_id_t)) {
		iface->frame++;
		(pxTaskWoken != NULL) ? csp_buffer_free_isr(frame) : csp_buffer_free(frame);
		return;
	}

	/* Strip the CSP header off the length field before converting to CSP packet */
	frame->len -= sizeof(csp_id_t);

	if (frame->len > csp_buffer_data_size()) { // consistency check, should never happen
		iface->rx_error++;
		(pxTaskWoken != NULL) ? csp_buffer_free_isr(frame) : csp_buffer_free(frame);
		return;
	}

	/* Convert the packet from network to host order */
	csp_packet_t * packet = (csp_packet_t *) frame;
	packet->id.ext = csp_ntoh32(packet->id.ext);

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

        const unsigned int max_data_size = csp_buffer_data_size();
        if ((iface->mtu == 0) || (iface->mtu > max_data_size)) {
		iface->mtu = max_data_size;
	}

        iface->nexthop = csp_i2c_tx;

	return csp_iflist_add(iface);
}
