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

#ifndef _CSP_IO_H_
#define _CSP_IO_H_

#include <csp/csp.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
   Send CSP packet via route (no existing connection).

   @param idout 32bit CSP identifier
   @param packet packet to send - this will not be freed.
   @param ifroute route to destination
   @param timeout timeout to wait for TX to complete. NOTE: not all underlying drivers supports flow-control.
   @return #CSP_ERR_NONE on success, otherwise an error code.
*/
int csp_send_direct(csp_id_t idout, csp_packet_t * packet, const csp_route_t * ifroute, uint32_t timeout);

#ifdef __cplusplus
}
#endif
#endif
