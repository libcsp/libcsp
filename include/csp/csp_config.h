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

/**
 * This is the default SVN version of the CSP configuration file.
 * It contains all the required values to compile CSP.
 * If you make any changes to the values in this file, please avoid
 * commiting them back to the repository unless they are required.
 *
 * This can be done by copying the file to another directory
 * and using include-path prioritisation, to prefer your local
 * copy over the default. Or perhaps even simpler by defining the
 * symbol CSP_USER_CONFIG in your makefile and naming your copy
 * csp_config_user.h
 *
 * This will also ensure that your copy of the configuraiton will never
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

#ifdef CSP_USER_CONFIG
#include <csp_config_user.h>
#else

/* General config */
#define CSP_DEBUG			   	0	   	// Enable/disable debugging output
#define CSP_CONN_MAX			10  	// Number of statically allocated connection structs
#define CSP_CONN_QUEUE_LENGTH	100		// Number of packets potentially in queue for a connection
#define CSP_FIFO_INPUT			100		// Number of packets to be queued at the input of the router
#define CSP_MAX_BIND_PORT		15		// Highest incoming port number to bind to (must be below (2^CSP_ID_PORT_SIZE)-1)
#define CSP_RANDOMIZE_EPHEM		1		// Randomize initial ephemeral port

/* Transport layer config */
#define CSP_USE_RDP				1		// Enable RDP transport protocol
#define CSP_DELAY_ACKS			1		// Use delayed acknowledgements
#define CSP_RDP_MAX_WINDOW		20		// Maximum RDP window size

/* Router config */
#define CSP_USE_PROMISC			1		// Enable promiscuous mode functions

/* Buffer config */
#define CSP_BUFFER_CALLOC		0		// Set to 1 to clear buffer at allocation
#define CSP_BUFFER_STATIC   	0		// Use a statically allocated buffer
#define CSP_BUFFER_SIZE		 	320		// Size of each buffer element
#define CSP_BUFFER_COUNT		12		// Number of buffer elements

/* Crypto config */
#define CSP_ENABLE_HMAC			0		// Enable Hash-based Message Authentication Code
#define CSP_ENABLE_XTEA			0		// Enable XTEA packet encryption

/* Key config - key must be 128 bit! */
#define CSP_CRYPTO_KEY			"\x31\x15\x49\x8a\x58\xc3\x01\x61\xe8\x33\x4a\xf0\x60\x6a\x41\xf4"
#define CSP_CRYPTO_KEY_LENGTH	16

#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // _CSP_CONFIG_H_
