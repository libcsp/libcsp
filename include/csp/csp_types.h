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

#ifndef CSP_TYPES_H_
#define CSP_TYPES_H_

/**
   @file
   Basic types.
*/

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <csp_autoconfig.h> // -> CSP_X defines (compile configuration)
#include <csp/csp_error.h>

#ifdef __cplusplus
extern "C" {
#endif

#if (CSP_BIG_ENDIAN && CSP_LITTLE_ENDIAN)
#error "Only define/set either CSP_BIG_ENDIAN or CSP_LITTLE_ENDIAN"
#endif

/**
   Reserved ports for CSP services.
*/
typedef enum {
	CSP_CMP				= 0,   //!< CSP management, e.g. memory, routes, stats
	CSP_PING			= 1,   //!< Ping - return ping
	CSP_PS				= 2,   //!< Current process list
	CSP_MEMFREE			= 3,   //!< Free memory
	CSP_REBOOT			= 4,   //!< Reboot, see #CSP_REBOOT_MAGIC and #CSP_REBOOT_SHUTDOWN_MAGIC
	CSP_BUF_FREE		= 5,   //!< Free CSP buffers
	CSP_UPTIME			= 6,   //!< Uptime
} csp_service_port_t;

/** Listen on all ports, primarily used with csp_bind() */
#define CSP_ANY				255

/**
   Message priority.
*/
typedef enum {
	CSP_PRIO_CRITICAL		= 0, //!< Critical
	CSP_PRIO_HIGH			= 1, //!< High
	CSP_PRIO_NORM			= 2, //!< Normal (default)
	CSP_PRIO_LOW			= 3, //!< Low
} csp_prio_t;

/**
   CSP identifier/header.
*/
typedef struct {
	uint8_t pri;
	uint8_t flags;
	uint16_t src;
	uint16_t dst;
	uint8_t dport;
	uint8_t sport;
} csp_id_t;

/**
   @defgroup CSP_HEADER_FLAGS CSP header flags.
   @{
*/
#define CSP_FRES1			0x80 //!< Reserved for future use
#define CSP_FRES2			0x40 //!< Reserved for future use
#define CSP_FRES3			0x20 //!< Reserved for future use
#define CSP_FFRAG			0x10 //!< Use fragmentation
#define CSP_FHMAC			0x08 //!< Use HMAC verification
#define CSP_FXTEA			0x04 //!< Use XTEA encryption
#define CSP_FRDP			0x02 //!< Use RDP protocol
#define CSP_FCRC32			0x01 //!< Use CRC32 checksum
/**@}*/

/**
   @defgroup CSP_SOCKET_OPTIONS CSP Socket options.
   @{
*/
#define CSP_SO_NONE			0x0000 //!< No socket options
#define CSP_SO_RDPREQ			0x0001 //!< Require RDP
#define CSP_SO_RDPPROHIB		0x0002 //!< Prohibit RDP
#define CSP_SO_HMACREQ			0x0004 //!< Require HMAC
#define CSP_SO_HMACPROHIB		0x0008 //!< Prohibit HMAC
#define CSP_SO_XTEAREQ			0x0010 //!< Require XTEA
#define CSP_SO_XTEAPROHIB		0x0020 //!< Prohibit HMAC
#define CSP_SO_CRC32REQ			0x0040 //!< Require CRC32
#define CSP_SO_CRC32PROHIB		0x0080 //!< Prohibit CRC32
#define CSP_SO_CONN_LESS		0x0100 //!< Enable Connection Less mode
#define CSP_SO_INTERNAL_LISTEN          0x1000 //!< Internal flag: listen called on socket
#define CSP_SO_CONN_LESS_CALLBACK 	0x0200 // Enable Callbacks directly in router task
#define CSP_SO_SAME			0x8000 // Copy opts from incoming packet only apllies to csp_sendto_reply()

/**@}*/

/** CSP Connect options */
#define CSP_O_NONE			CSP_SO_NONE        //!< No connection options
#define CSP_O_RDP			CSP_SO_RDPREQ      //!< Enable RDP
#define CSP_O_NORDP			CSP_SO_RDPPROHIB   //!< Disable RDP
#define CSP_O_HMAC			CSP_SO_HMACREQ     //!< Enable HMAC
#define CSP_O_NOHMAC			CSP_SO_HMACPROHIB  //!< Disable HMAC
#define CSP_O_XTEA			CSP_SO_XTEAREQ     //!< Enable XTEA
#define CSP_O_NOXTEA			CSP_SO_XTEAPROHIB  //!< Disable XTEA
#define CSP_O_CRC32			CSP_SO_CRC32REQ    //!< Enable CRC32
#define CSP_O_NOCRC32			CSP_SO_CRC32PROHIB //!< Disable CRC32
#define CSP_O_SAME			CSP_SO_SAME

//doc-begin:csp_packet_t
/**
   CSP Packet.

   This structure is constructed to fit with all interface and protocols to prevent the
   need to copy data (zero copy).

   @note In most cases a CSP packet cannot be reused in case of send failure, because the
   lower layers may add additional data causing increased length (e.g. CRC32), convert
   the CSP id to different endian (e.g. I2C), etc.
*/
typedef struct {
	uint32_t rdp_quarantine;	// EACK quarantine period
	uint32_t timestamp_tx;		// Time the message was sent
	uint32_t timestamp_rx;		// Time the message was received

	uint16_t length;			// Data length
	csp_id_t id;				// CSP id (unpacked version CPU readable)

	uint8_t * frame_begin;
	uint16_t frame_length;

	uint8_t header[8];  // Additional header bytes, to prepend packed data before transmission
	/**
	 * Data part of packet.
	 * When using the csp_buffer API, the size of the data part is set by
	 * csp_buffer_init(), and can later be accessed by csp_buffer_data_size()
	 */
	union {
		/** Access data as uint8_t. */
		uint8_t data[0];
		/** Access data as uint16_t */
		uint16_t data16[0];
		/** Access data as uint32_t */
		uint32_t data32[0];
	};

} csp_packet_t;
//doc-end:csp_packet_t

/**
   Size of the packet overhead in #csp_packet_t.
   The overhead is the difference between the total buffer size (returned by csp_buffer_size()) and the data part
   of the #csp_packet_t (returned by csp_buffer_data_size()).
*/
#define CSP_BUFFER_PACKET_OVERHEAD      (sizeof(csp_packet_t) - sizeof(((csp_packet_t *)0)->data))

/** Forward declaration of CSP interface, see #csp_iface_s for details. */
typedef struct csp_iface_s csp_iface_t;
/** Forward declaration of outgoing CSP route, see #csp_route_s for details. */
typedef struct csp_route_s csp_route_t;

/** Forward declaration of socket structure */
typedef struct csp_conn_s csp_socket_t;
/** Forward declaration of connection structure */
typedef struct csp_conn_s csp_conn_t;

/** Max length of host name - including zero termination */
#define CSP_HOSTNAME_LEN	20
/** Max length of model name - including zero termination */
#define CSP_MODEL_LEN		30

/** Magic number for reboot request, for service-code #CSP_REBOOT */
#define CSP_REBOOT_MAGIC		0x80078007
/** Magic number for shutdown request, for service-code #CSP_REBOOT */
#define CSP_REBOOT_SHUTDOWN_MAGIC	0xD1E5529A

#ifdef __AVR__
typedef uint32_t csp_memptr_t;
typedef const uint32_t csp_const_memptr_t;
#else
/** Memory pointer */
typedef void * csp_memptr_t;
/** Const memory pointer */
typedef const void * csp_const_memptr_t;
#endif

/**
   Platform specific memory copy function.
*/
typedef csp_memptr_t (*csp_memcpy_fnc_t)(csp_memptr_t, csp_const_memptr_t, size_t);

/**
   Compile check/asserts.
*/
#define CSP_STATIC_ASSERT(condition, name)   typedef char name[(condition) ? 1 : -1]
    
#ifdef __cplusplus
}
#endif
#endif /* CSP_TYPES_H_ */
