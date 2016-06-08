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

#include <csp/csp_autoconfig.h>

/* Make bool for compilers without stdbool.h */
#ifdef CSP_HAVE_STDBOOL_H
#include <stdbool.h>
#else
#define bool int
#define false 0
#define true !false
#endif

/**
 * RESERVED PORTS (SERVICES)
 */

enum csp_reserved_ports_e {
	CSP_CMP				= 0,
	CSP_PING			= 1,
	CSP_PS				= 2,
	CSP_MEMFREE			= 3,
	CSP_REBOOT			= 4,
	CSP_BUF_FREE			= 5,
	CSP_UPTIME			= 6,
	CSP_ANY				= (CSP_MAX_BIND_PORT + 1),
	CSP_PROMISC			= (CSP_MAX_BIND_PORT + 2)
};

typedef enum {
	CSP_PRIO_CRITICAL		= 0,
	CSP_PRIO_HIGH			= 1,
	CSP_PRIO_NORM			= 2,
	CSP_PRIO_LOW			= 3,
} csp_prio_t;

#define CSP_PRIORITIES			(1 << CSP_ID_PRIO_SIZE)

#ifdef CSP_USE_QOS
#define CSP_RX_QUEUE_LENGTH		(CSP_CONN_QUEUE_LENGTH / CSP_PRIORITIES)
#define CSP_ROUTE_FIFOS			CSP_PRIORITIES
#define CSP_RX_QUEUES			CSP_PRIORITIES
#else
#define CSP_RX_QUEUE_LENGTH		CSP_CONN_QUEUE_LENGTH
#define CSP_ROUTE_FIFOS			1
#define CSP_RX_QUEUES			1
#endif

/** Size of bit-fields in CSP header */
#define CSP_ID_PRIO_SIZE		2
#define CSP_ID_HOST_SIZE		5
#define CSP_ID_PORT_SIZE		6
#define CSP_ID_FLAGS_SIZE		8

#define CSP_HEADER_BITS			(CSP_ID_PRIO_SIZE + 2 * CSP_ID_HOST_SIZE + 2 * CSP_ID_PORT_SIZE + CSP_ID_FLAGS_SIZE)
#define CSP_HEADER_LENGTH		(CSP_HEADER_BITS / 8)

#if CSP_HEADER_BITS != 32 && __GNUC__
#error "Header length must be 32 bits"
#endif

/** Highest number to be entered in field */
#define CSP_ID_PRIO_MAX			((1 << (CSP_ID_PRIO_SIZE)) - 1)
#define CSP_ID_HOST_MAX			((1 << (CSP_ID_HOST_SIZE)) - 1)
#define CSP_ID_PORT_MAX			((1 << (CSP_ID_PORT_SIZE)) - 1)
#define CSP_ID_FLAGS_MAX		((1 << (CSP_ID_FLAGS_SIZE)) - 1)

/** Identifier field masks */
#define CSP_ID_PRIO_MASK		((uint32_t) CSP_ID_PRIO_MAX << (CSP_ID_FLAGS_SIZE + 2 * CSP_ID_PORT_SIZE + 2 * CSP_ID_HOST_SIZE))
#define CSP_ID_SRC_MASK	 		((uint32_t) CSP_ID_HOST_MAX << (CSP_ID_FLAGS_SIZE + 2 * CSP_ID_PORT_SIZE + 1 * CSP_ID_HOST_SIZE))
#define CSP_ID_DST_MASK	 		((uint32_t) CSP_ID_HOST_MAX << (CSP_ID_FLAGS_SIZE + 2 * CSP_ID_PORT_SIZE))
#define CSP_ID_DPORT_MASK   		((uint32_t) CSP_ID_PORT_MAX << (CSP_ID_FLAGS_SIZE + 1 * CSP_ID_PORT_SIZE))
#define CSP_ID_SPORT_MASK   		((uint32_t) CSP_ID_PORT_MAX << (CSP_ID_FLAGS_SIZE))
#define CSP_ID_FLAGS_MASK		((uint32_t) CSP_ID_FLAGS_MAX << (0))

#define CSP_ID_CONN_MASK		(CSP_ID_SRC_MASK | CSP_ID_DST_MASK | CSP_ID_DPORT_MASK | CSP_ID_SPORT_MASK)

/** @brief This union defines a CSP identifier and allows access to the individual fields or the entire identifier */
typedef union {
	uint32_t ext;
	struct __attribute__((__packed__)) {
#if defined(CSP_BIG_ENDIAN) && !defined(CSP_LITTLE_ENDIAN)
		unsigned int pri : CSP_ID_PRIO_SIZE;
		unsigned int src : CSP_ID_HOST_SIZE;
		unsigned int dst : CSP_ID_HOST_SIZE;
		unsigned int dport : CSP_ID_PORT_SIZE;
		unsigned int sport : CSP_ID_PORT_SIZE;
		unsigned int flags : CSP_ID_FLAGS_SIZE;
#elif defined(CSP_LITTLE_ENDIAN) && !defined(CSP_BIG_ENDIAN)
		unsigned int flags : CSP_ID_FLAGS_SIZE;
		unsigned int sport : CSP_ID_PORT_SIZE;
		unsigned int dport : CSP_ID_PORT_SIZE;
		unsigned int dst : CSP_ID_HOST_SIZE;
		unsigned int src : CSP_ID_HOST_SIZE;
		unsigned int pri : CSP_ID_PRIO_SIZE;
#else
		#error "Must define one of CSP_BIG_ENDIAN or CSP_LITTLE_ENDIAN in csp_platform.h"
#endif
	};
} csp_id_t;

/** Broadcast address */
#define CSP_BROADCAST_ADDR		CSP_ID_HOST_MAX

/** Default routing address */
#define CSP_DEFAULT_ROUTE		(CSP_ID_HOST_MAX + 1)

/** CSP Flags */
#define CSP_FRES1			0x80 // Reserved for future use
#define CSP_FRES2			0x40 // Reserved for future use
#define CSP_FRES3			0x20 // Reserved for future use
#define CSP_FFRAG			0x10 // Use fragmentation
#define CSP_FHMAC			0x08 // Use HMAC verification
#define CSP_FXTEA			0x04 // Use XTEA encryption
#define CSP_FRDP			0x02 // Use RDP protocol
#define CSP_FCRC32			0x01 // Use CRC32 checksum

/** CSP Socket options */
#define CSP_SO_NONE			0x0000 // No socket options
#define CSP_SO_RDPREQ			0x0001 // Require RDP
#define CSP_SO_RDPPROHIB		0x0002 // Prohibit RDP
#define CSP_SO_HMACREQ			0x0004 // Require HMAC
#define CSP_SO_HMACPROHIB		0x0008 // Prohibit HMAC
#define CSP_SO_XTEAREQ			0x0010 // Require XTEA
#define CSP_SO_XTEAPROHIB		0x0020 // Prohibit HMAC
#define CSP_SO_CRC32REQ			0x0040 // Require CRC32
#define CSP_SO_CRC32PROHIB		0x0080 // Prohibit CRC32
#define CSP_SO_CONN_LESS		0x0100 // Enable Connection Less mode

/** CSP Connect options */
#define CSP_O_NONE			CSP_SO_NONE // No connection options
#define CSP_O_RDP			CSP_SO_RDPREQ // Enable RDP
#define CSP_O_NORDP			CSP_SO_RDPPROHIB // Disable RDP
#define CSP_O_HMAC			CSP_SO_HMACREQ // Enable HMAC
#define CSP_O_NOHMAC			CSP_SO_HMACPROHIB // Disable HMAC
#define CSP_O_XTEA			CSP_SO_XTEAREQ // Enable XTEA
#define CSP_O_NOXTEA			CSP_SO_XTEAPROHIB // Disable XTEA
#define CSP_O_CRC32			CSP_SO_CRC32REQ // Enable CRC32
#define CSP_O_NOCRC32			CSP_SO_CRC32PROHIB // Disable CRC32

/**
 * CSP PACKET STRUCTURE
 * Note: This structure is constructed to fit
 * with all interface frame types in order to
 * have buffer reuse
 */
typedef struct __attribute__((__packed__)) {
	uint8_t padding[CSP_PADDING_BYTES];	/**< Interface dependent padding */
	uint16_t length;			/**< Length field must be just before CSP ID */
	csp_id_t id;				/**< CSP id must be just before data */
	union {
		uint8_t data[0];		/**< This just points to the rest of the buffer, without a size indication. */
		uint16_t data16[0];		/**< The data 16 and 32 types makes it easy to reference an integer (properly aligned) */
		uint32_t data32[0];		/**< without the compiler warning about strict aliasing rules. */
	};
} csp_packet_t;

/** Interface TX function */
struct csp_iface_s;
typedef int (*nexthop_t)(struct csp_iface_s * interface, csp_packet_t *packet, uint32_t timeout);

/** Interface struct */
typedef struct csp_iface_s {
	const char *name;			/**< Interface name (keep below 10 bytes) */
	void * driver;				/**< Pointer to interface handler structure */
	nexthop_t nexthop;			/**< Next hop function */
	uint16_t mtu;				/**< Maximum Transmission Unit of interface */
	uint8_t split_horizon_off;	/**< Disable the route-loop prevention on if */
	uint32_t tx;				/**< Successfully transmitted packets */
	uint32_t rx;				/**< Successfully received packets */
	uint32_t tx_error;			/**< Transmit errors */
	uint32_t rx_error;			/**< Receive errors */
	uint32_t drop;				/**< Dropped packets */
	uint32_t autherr; 			/**< Authentication errors */
	uint32_t frame;				/**< Frame format errors */
	uint32_t txbytes;			/**< Transmitted bytes */
	uint32_t rxbytes;			/**< Received bytes */
	uint32_t irq;				/**< Interrupts */
	struct csp_iface_s *next;	/**< Next interface */
} csp_iface_t;

/**
 * This define must be equal to the size of the packet overhead in csp_packet_t.
 * It is used in csp_buffer_get() to check the allocated buffer size against
 * the required buffer size.
 */
#define CSP_BUFFER_PACKET_OVERHEAD 	(sizeof(csp_packet_t) - sizeof(((csp_packet_t *)0)->data))

/** Forward declaration of socket and connection structures */
typedef struct csp_conn_s csp_socket_t;
typedef struct csp_conn_s csp_conn_t;

#define CSP_HOSTNAME_LEN	20
#define CSP_MODEL_LEN		30

/* CSP_REBOOT magic values */
#define CSP_REBOOT_MAGIC		0x80078007
#define CSP_REBOOT_SHUTDOWN_MAGIC	0xD1E5529A

#endif /* CSP_TYPES_H_ */
