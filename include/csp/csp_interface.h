/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2012 Gomspace ApS (http://www.gomspace.com)
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

#ifndef _CSP_INTERFACE_H_
#define _CSP_INTERFACE_H_

/**
   @file
   Interface.
*/

#include <csp/csp_platform.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
   Max unique length of interface name, when matching names.
*/
#define CSP_IFLIST_NAME_MAX    10

/**
   Interface Tx function.

   @param[in] ifroute contains the interface and the \a mac adddress.
   @param[in] packet CSP packet to send. On success, the packet must be freed using csp_buffer_free().
   @return #CSP_ERR_NONE on success, otherwise an error code.
*/
typedef int (*nexthop_t)(const csp_route_t * ifroute, csp_packet_t *packet);

//doc-begin:csp_iface_s
/**
   CSP interface.
*/
struct csp_iface_s {
    const char *name;          //!< Name, max compare length is #CSP_IFLIST_NAME_MAX
    void * interface_data;     //!< Interface data, only known/used by the interface layer, e.g. state information.
    void * driver_data;        //!< Driver data, only known/used by the driver layer, e.g. device/channel references.
    nexthop_t nexthop;         //!< Next hop (Tx) function
    uint16_t mtu;              //!< Maximum Transmission Unit of interface
    uint8_t split_horizon_off; //!< Disable the route-loop prevention
    uint32_t tx;               //!< Successfully transmitted packets
    uint32_t rx;               //!< Successfully received packets
    uint32_t tx_error;         //!< Transmit errors (packets)
    uint32_t rx_error;         //!< Receive errors, e.g. too large message
    uint32_t drop;             //!< Dropped packets
    uint32_t autherr;          //!< Authentication errors (packets)
    uint32_t frame;            //!< Frame format errors (packets)
    uint32_t txbytes;          //!< Transmitted bytes
    uint32_t rxbytes;          //!< Received bytes
    uint32_t irq;              //!< Interrupts
    struct csp_iface_s *next;  //!< Internal, interfaces are stored in a linked list
};
//doc-end:csp_iface_s

/**
   Inputs a new packet into the system.

   This function can be called from interface drivers (ISR) or tasks, to route and accept packets.

   @note EXTREMELY IMPORTANT: \a pxTaskWoken must ALWAYS be NULL if called from task, and ALWAYS
   be NON NULL if called from ISR. If this condition is met, this call is completely thread-safe

   This function is fire and forget, it returns void, meaning that the \a packet will always be
   either accepted or dropped, so the memory will always be freed.

   @param[in] packet A pointer to the incoming packet
   @param[in] iface A pointer to the incoming interface TX function.
   @param[out] pxTaskWoken Valid reference if called from ISR, otherwise NULL!
*/
void csp_qfifo_write(csp_packet_t *packet, csp_iface_t *iface, CSP_BASE_TYPE *pxTaskWoken);

#ifdef __cplusplus
}
#endif
#endif

