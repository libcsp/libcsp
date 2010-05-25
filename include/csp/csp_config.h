/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2010 Gomspace ApS (gomspace.com)
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

#ifndef _CSP_CONFIG_H_
#define _CSP_CONFIG_H_

/* General config */
#define CSP_DEBUG           0       // Enable/disable debugging output
#define CSP_MTU             260     // Maximum CSP packet size, including header
#define CONN_MAX			10      // Number of statically allocated connection structs
#define CONN_QUEUE_LENGTH	100		// Number of packets potentially in queue for a connection

/* Buffer config */
#define CSP_BUFFER_STATIC   0
#define CSP_BUFFER_SIZE     320
#define CSP_BUFFER_COUNT    12
#define CSP_BUFFER_FREE	    0  
#define CSP_BUFFER_USED	    1

#endif // _CSP_CONFIG_H_
