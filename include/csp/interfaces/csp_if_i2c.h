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

#ifndef _CSP_IF_I2C_H_
#define _CSP_IF_I2C_H_

/**
   @file

   I2C interface.
*/

#include <csp/csp_interface.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
   Default name of I2C interface.
*/
#define CSP_IF_I2C_DEFAULT_NAME "I2C"

//doc-begin:csp_i2c_frame_t
/**
   I2C frame.
   This struct fits on top of a #csp_packet_t, removing the need for copying data.
*/
typedef struct i2c_frame_s {
    //! Not used  (-> csp_packet_t.padding)
    uint8_t padding[3];
    //! Cleared before Tx  (-> csp_packet_t.padding)
    uint8_t retries;
    //! Not used  (-> csp_packet_t.padding)
    uint32_t reserved;
    //! Destination address  (-> csp_packet_t.padding)
    uint8_t dest;
    //! Cleared before Tx  (-> csp_packet_t.padding)
    uint8_t len_rx;
    //! Length of \a data part  (-> csp_packet_t.length)
    uint16_t len;
    //! CSP id + data  (-> csp_packet_t.id)
    uint8_t data[0];
} csp_i2c_frame_t;
//doc-end:csp_i2c_frame_t

/**
   Send I2C frame (implemented by driver).

   Used by csp_i2c_tx() to send a frame.

   The function must free the frame/packet using csp_buffer_free(), if the send succeeds (returning #CSP_ERR_NONE).

   @param[in] driver_data driver data from #csp_iface_t
   @param[in] frame destination, length and data. This is actually a #csp_packet_t buffer, casted to #csp_i2c_frame_t.
   @return #CSP_ERR_NONE on success, or an error code.
*/
typedef int (*csp_i2c_driver_tx_t)(void * driver_data, csp_i2c_frame_t * frame);

/**
   Interface data (state information).
*/
typedef struct {
    /** Tx function */
    csp_i2c_driver_tx_t tx_func;
} csp_i2c_interface_data_t;

/**
   Add interface.

   @param[in] iface CSP interface, initialized with name and inteface_data pointing to a valid #csp_i2c_interface_data_t.
   @return #CSP_ERR_NONE on success, otherwise an error code.
*/
int csp_i2c_add_interface(csp_iface_t * iface);

/**
   Send CSP packet over I2C (nexthop).

   @param[in] ifroute route.
   @param[in] packet CSP packet to send.
   @return #CSP_ERR_NONE on success, otherwise an error code.
*/
int csp_i2c_tx(const csp_route_t * ifroute, csp_packet_t * packet);

/**
   Process received I2C frame.

   @note The received #csp_i2c_frame_t must actually be pointing to an #csp_packet_t.

   Called from driver, when a frame has been received.

   @param[in] iface incoming interface.
   @param[in] frame received data, routed on as a #csp_packet_t.
   @param[out] pxTaskWoken Valid reference if called from ISR, otherwise NULL!
*/
void csp_i2c_rx(csp_iface_t * iface, csp_i2c_frame_t * frame, void * pxTaskWoken);

#ifdef __cplusplus
}
#endif
#endif
