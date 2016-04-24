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
#include <stdint.h>

#include <csp/csp.h>
#include <csp/csp_endian.h>
#include <csp/csp_interface.h>
#include <csp/csp_error.h>
#include <csp/interfaces/csp_if_i2c.h>
#include <csp/drivers/i2c.h>

static int csp_i2c_handle = 0;

int csp_i2c_tx(csp_iface_t * interface, csp_packet_t * packet, uint32_t timeout) {

	/* Cast the CSP packet buffer into an i2c frame */
	i2c_frame_t * frame = (i2c_frame_t *) packet;

	/* Insert destination node into the i2c destination field */
	if (csp_rtable_find_mac(packet->id.dst) == CSP_NODE_MAC) {
		frame->dest = packet->id.dst;
	} else {
		frame->dest = csp_rtable_find_mac(packet->id.dst);
	}

	/* Save the outgoing id in the buffer */
	packet->id.ext = csp_hton32(packet->id.ext);

	/* Add the CSP header to the I2C length field */
	frame->len += sizeof(packet->id);
	frame->len_rx = 0;

	/* Some I2C drivers support X number of retries
	 * CSP don't care about this. If it doesn't work the first
	 * time, don'y use time on it.
	 */
	frame->retries = 0;

	/* enqueue the frame */
	if (i2c_send(csp_i2c_handle, frame, timeout) != E_NO_ERR)
		return CSP_ERR_DRIVER;

	return CSP_ERR_NONE;

}

/**
 * When a frame is received, cast it to a csp_packet
 * and send it directly to the CSP new packet function.
 * Context: ISR only
 * @param frame
 */
void csp_i2c_rx(i2c_frame_t * frame, void * pxTaskWoken) {

	static csp_packet_t * packet;

	/* Validate input */
	if (frame == NULL)
		return;

	if ((frame->len < 4) || (frame->len > I2C_MTU)) {
		csp_if_i2c.frame++;
		csp_buffer_free_isr(frame);
		return;
	}

	/* Strip the CSP header off the length field before converting to CSP packet */
	frame->len -= sizeof(csp_id_t);

	/* Convert the packet from network to host order */
	packet = (csp_packet_t *) frame;
	packet->id.ext = csp_ntoh32(packet->id.ext);

	/* Receive the packet in CSP */
	csp_new_packet(packet, &csp_if_i2c, pxTaskWoken);

}

int csp_i2c_init(uint8_t addr, int handle, int speed) {

	/* Create i2c_handle */
	csp_i2c_handle = handle;
	if (i2c_init(csp_i2c_handle, I2C_MASTER, addr, speed, 10, 10, csp_i2c_rx) != E_NO_ERR)
		return CSP_ERR_DRIVER;

	/* Register interface */
	csp_iflist_add(&csp_if_i2c);

	return CSP_ERR_NONE;

}

/** Interface definition */
csp_iface_t csp_if_i2c = {
	.name = "I2C",
	.nexthop = csp_i2c_tx,
};
