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

#include <csp/csp_autoconfig.h> // -> CSP_X defines (compile configuration)
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
	CSP_BUF_FREE			= 5,   //!< Free CSP buffers
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

#define CSP_PRIORITIES			(1 << CSP_ID_PRIO_SIZE) //!< Number of CSP message priorities.

#if (CSP_USE_QOS || __DOXYGEN__)
#define CSP_ROUTE_FIFOS			CSP_PRIORITIES //!< Number of fifos for incoming messages (handover to router)
#define CSP_RX_QUEUES			CSP_PRIORITIES //!< Number of fifos for incoming message per message-queue
#else
#define CSP_ROUTE_FIFOS			1
#define CSP_RX_QUEUES			1
#endif

/**
   @defgroup CSP_HEADER_DEF CSP header definition.
   @{
*/
#define CSP_ID_PRIO_SIZE		2 //!< Bits for priority, see #csp_prio_t
#define CSP_ID_HOST_SIZE		5 //!< Bits for host (destination/source address)
#define CSP_ID_PORT_SIZE		6 //!< Bits for port (destination/source port)
#define CSP_ID_FLAGS_SIZE		8 //!< Bits for flags, see @ref CSP_HEADER_FLAGS

/** Number of bits in CSP header */
#define CSP_HEADER_BITS			(CSP_ID_PRIO_SIZE + (2 * CSP_ID_HOST_SIZE) + (2 * CSP_ID_PORT_SIZE) + CSP_ID_FLAGS_SIZE)
/** CSP header size in bytes */
#define CSP_HEADER_LENGTH		(CSP_HEADER_BITS / 8)

#if CSP_HEADER_BITS != 32 && __GNUC__
#error "Header length must be 32 bits"
#endif

#define CSP_ID_PRIO_MAX			((1 << (CSP_ID_PRIO_SIZE)) - 1)  //!< Max priority value in header
#define CSP_ID_HOST_MAX			((1 << (CSP_ID_HOST_SIZE)) - 1)  //!< Max host value in header
#define CSP_ID_PORT_MAX			((1 << (CSP_ID_PORT_SIZE)) - 1)  //!< Max port value in header
#define CSP_ID_FLAGS_MAX		((1 << (CSP_ID_FLAGS_SIZE)) - 1) //!< Max flag(s) value in header

/** CSP identifier/header - priority mask */
#define CSP_ID_PRIO_MASK		((uint32_t) CSP_ID_PRIO_MAX  << (CSP_ID_FLAGS_SIZE + (2 * CSP_ID_PORT_SIZE) + (2 * CSP_ID_HOST_SIZE)))
/** CSP identifier/header - source address mask */
#define CSP_ID_SRC_MASK	 		((uint32_t) CSP_ID_HOST_MAX  << (CSP_ID_FLAGS_SIZE + (2 * CSP_ID_PORT_SIZE) + (1 * CSP_ID_HOST_SIZE)))
/** CSP identifier/header - destination address mask */
#define CSP_ID_DST_MASK	 		((uint32_t) CSP_ID_HOST_MAX  << (CSP_ID_FLAGS_SIZE + (2 * CSP_ID_PORT_SIZE)))
/** CSP identifier/header - destination port mask */
#define CSP_ID_DPORT_MASK   		((uint32_t) CSP_ID_PORT_MAX  << (CSP_ID_FLAGS_SIZE + (1 * CSP_ID_PORT_SIZE)))
/** CSP identifier/header - source port mask */
#define CSP_ID_SPORT_MASK   		((uint32_t) CSP_ID_PORT_MAX  << (CSP_ID_FLAGS_SIZE))
/** CSP identifier/header - flag mask */
#define CSP_ID_FLAGS_MASK		((uint32_t) CSP_ID_FLAGS_MAX << (0))
/** CSP identifier/header - connection mask (source & destination address + source & destination ports) */
#define CSP_ID_CONN_MASK		(CSP_ID_SRC_MASK | CSP_ID_DST_MASK | CSP_ID_DPORT_MASK | CSP_ID_SPORT_MASK)
/**@}*/

/**
   CSP identifier/header.
   This union is sent directly on the wire, hence the big/little endian definitions
*/
typedef union {
    /** Entire identifier. */
    uint32_t ext;
    /** Individual fields. */
    struct __attribute__((__packed__)) {
#if (CSP_BIG_ENDIAN || __DOXYGEN__)
        unsigned int pri   : CSP_ID_PRIO_SIZE;  //!< Priority
        unsigned int src   : CSP_ID_HOST_SIZE;  //!< Source address
        unsigned int dst   : CSP_ID_HOST_SIZE;  //!< Destination address
        unsigned int dport : CSP_ID_PORT_SIZE;  //!< Destination port
        unsigned int sport : CSP_ID_PORT_SIZE;  //!< Source port
        unsigned int flags : CSP_ID_FLAGS_SIZE; //!< Flags, see @ref CSP_HEADER_FLAGS
#elif (CSP_LITTLE_ENDIAN)
        unsigned int flags : CSP_ID_FLAGS_SIZE;
        unsigned int sport : CSP_ID_PORT_SIZE;
        unsigned int dport : CSP_ID_PORT_SIZE;
        unsigned int dst   : CSP_ID_HOST_SIZE;
        unsigned int src   : CSP_ID_HOST_SIZE;
        unsigned int pri   : CSP_ID_PRIO_SIZE;
#endif
    };
} csp_id_t;

/** Broadcast address */
#define CSP_BROADCAST_ADDR		CSP_ID_HOST_MAX

/** Default routing address */
#define CSP_DEFAULT_ROUTE		(CSP_ID_HOST_MAX + 1)

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
/**@}*/

/**
   @defgroup CSP_CONNECTION_OPTIONS CSP Connect options.
   @{
*/
#define CSP_O_NONE			CSP_SO_NONE        //!< No connection options
#define CSP_O_RDP			CSP_SO_RDPREQ      //!< Enable RDP
#define CSP_O_NORDP			CSP_SO_RDPPROHIB   //!< Disable RDP
#define CSP_O_HMAC			CSP_SO_HMACREQ     //!< Enable HMAC
#define CSP_O_NOHMAC			CSP_SO_HMACPROHIB  //!< Disable HMAC
#define CSP_O_XTEA			CSP_SO_XTEAREQ     //!< Enable XTEA
#define CSP_O_NOXTEA			CSP_SO_XTEAPROHIB  //!< Disable XTEA
#define CSP_O_CRC32			CSP_SO_CRC32REQ    //!< Enable CRC32
#define CSP_O_NOCRC32			CSP_SO_CRC32PROHIB //!< Disable CRC32
/**@}*/

/**
   Padding size in #csp_packet_t.
   10 bytes ensure correct aligned \a id and \a data in #csp_packet_t.
*/
#define CSP_PADDING_BYTES		10

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
	/**
           Padding. These bytes are intended for use by protocols, which want to prepend
           data before sending it, without having to copy/reorganize the entire message.
        */
        uint8_t padding[CSP_PADDING_BYTES];
        /** Data length. Must be just before CSP ID.*/
	uint16_t length;
	/** CSP id. Must be just before data, as it allows the interface to id and data
            in a single operation. */
	csp_id_t id;
	/**
           Data part of packet.
           When using the csp_buffer API, the size of the data part is set by
           csp_buffer_init(), and can later be accessed by csp_buffer_data_size()
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
