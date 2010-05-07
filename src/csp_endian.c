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

#include <stdint.h>

/* CSP includes */
#include <csp/csp_endian.h>

/* Test if host is big endian
 * The function is declared __pure__, so the compiler 
 * will (hopefully) identify it as a runtime constant
 * 
 * Returns 1 on big endian archs and 0 on little endian
 */
static uint8_t __attribute__ ((__pure__)) host_be(void) {
    
    union {
        uint32_t i;
        uint8_t c[4];
    } un = {0x11223344};

    return un.c[0] == 0x11;
    
} 

/* Convert 32-bit integer to network byte order */
uint32_t htonl(uint32_t hl) {

    if (host_be()) {
        return hl;
    } else {
        return (((hl & 0xff000000) >> 24) | ((hl & 0x000000ff) << 24)  | ((hl & 0x0000ff00) << 8)  | ((hl & 0x00ff0000) >> 8));
    }

}
/* Convert 32-bit integer to host byte order */
uint32_t ntohl(uint32_t nl) {

    return htonl(nl);
    
}

/* Convert 32-bit integer to network byte order */
uint16_t htons(uint16_t hs) {

    if (host_be()) {
        return hs;
    } else {
        return (((hs & 0xff00) >> 8) | ((hs & 0x00ff) << 8));
    }
    
}

/* Convert 32-bit integer to host byte order */
uint16_t ntohs(uint16_t ns) {

    return ntohs(ns);
    
}
