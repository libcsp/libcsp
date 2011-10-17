/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2011 Gomspace ApS (http://www.gomspace.com)
Copyright (C) 2011 AAUSAT3 Project (http://aausat3.space.aau.dk) 

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

/*
 * This is the default version of the CSP configuration file. It
 * contains all the required values to compile CSP. If you make
 * any changes to the values in this file, please avoid committing
 * them back to the repository unless they are required.
 *
 * This can be done by copying the file to another directory
 * and using include-path prioritization, to prefer your local
 * copy over the default. Or perhaps even simpler by defining the
 * symbol CSP_USER_CONFIG in your makefile and naming your copy
 * csp_config_user.h
 *
 * This will also ensure that your copy of the configuration will never
 * be overwritten by a SVN checkout. However, please notice that
 * sometimes new configuration directives will be added to the configuration
 * at which point you should copy these to your local configuration too.
 *
 */

#ifndef _CSP_CONFIG_H_
#define _CSP_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CSP_CONFIG
	#include CSP_CONFIG
#else
	#include "csp_config_dfl.h"
#endif

/* General config */
#ifndef CSP_DEBUG
	#error "Missing config option: CSP_DEBUG"
#endif

#ifndef CSP_CONN_MAX
	#error "Missing config option: CSP_CONN_MAX"
#endif

#ifndef CSP_CONN_QUEUE_LENGTH
	#error "Missing config option: CSP_CONN_QUEUE_LENGTH"
#endif

#ifndef CSP_FIFO_INPUT
	#error "Missing config option: CSP_FIFO_INPUT"
#endif

#ifndef CSP_MAX_BIND_PORT
	#error "Missing config option: CSP_MAX_BIND_PORT"
#endif

#ifndef CSP_USE_QOS
	#error "Missing config option: CSP_USE_QOS"
#endif

/* Transport layer config */
#ifndef CSP_USE_RDP
	#error "Missing config option: CSP_USE_RDP"
#endif

#ifndef CSP_RDP_MAX_WINDOW
	#error "Missing config option: CSP_RDP_MAX_WINDOW"
#endif

/* Router config */
#ifndef CSP_USE_PROMISC
	#error "Missing config option: CSP_USE_PROMISC"
#endif

/* Buffer config */
#ifndef CSP_BUFFER_CALLOC
	#error "Missing config option: CSP_BUFFER_CALLOC"
#endif

#ifndef CSP_BUFFER_STATIC
	#error "Missing config option: CSP_BUFFER_STATIC"
#endif

#ifndef CSP_BUFFER_SIZE
	#error "Missing config option: CSP_BUFFER_SIZE"
#endif

#ifndef CSP_BUFFER_COUNT
	#error "Missing config option: CSP_BUFFER_COUNT"
#endif

/* CRC32 config */
#ifndef CSP_ENABLE_CRC32
	#error "Missing config option: CSP_ENABLE_CRC32"
#endif

/* Crypto config */
#ifndef CSP_ENABLE_HMAC
	#error "Missing config option: CSP_ENABLE_HMAC"
#endif

#ifndef CSP_ENABLE_XTEA
	#error "Missing config option: CSP_ENABLE_XTEA"
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // _CSP_CONFIG_H_
