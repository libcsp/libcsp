/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2010 GomSpace ApS (gomspace.com)
Copyright (C) 2010 AAUSAT3 Project (aausat3.space.aau.dk) 

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

#ifndef _CSP_ROUTE_H_
#define _CSP_ROUTE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include <csp/csp.h>

#include "arch/csp_thread.h"

typedef struct {
    const char * name;
    nexthop_t nexthop;
    uint8_t nexthop_mac_addr;
    uint16_t count;
} csp_iface_t;

void csp_route_table_init(void);
csp_iface_t * csp_route_if(uint8_t id);
csp_conn_t * csp_route(csp_id_t id, nexthop_t interface, CSP_BASE_TYPE * pxTaskWoken);
csp_thread_return_t vTaskCSPRouter(void * pvParameters);

#if CSP_USE_PROMISC
void csp_promisc_add(csp_packet_t * packet, csp_queue_handle_t queue);
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // _CSP_ROUTE_H_
