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

#ifndef _CSP_H_
#define _CSP_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes */
#include <stdint.h>
#include <stdlib.h>

/* CSP includes */
#include "csp_platform.h"
#include "csp_config.h"

/* todo: move this include to the files that actually use them */
#include "csp_buffer.h"

/** The address of the node */
extern uint8_t my_address;

/** Define used to specify MAC_ADDR = NODE_ID */
#define CSP_NODE_MAC			0xFF

/**
 * RESERVED PORTS (SERVICES)
 */

enum csp_reserved_ports_e {
	CSP_PING			= 1,
	CSP_PS			  	= 2,
	CSP_MEMFREE		 	= 3,
	CSP_REBOOT		  	= 4,
	CSP_BUF_FREE		= 5,
	CSP_UPTIME			= 6,
	CSP_STATS			= 7,
	CSP_ANY			 	= (CSP_MAX_BIND_PORT + 1),
	CSP_PROMISC		 	= (CSP_MAX_BIND_PORT + 2)
};

#if CSP_HDR_0_9

/**
 * PRIORITIES
 */

typedef enum csp_prio_e {
        CSP_PRIO_CRITICAL   = 0,
        CSP_PRIO_ALERT      = 1,
        CSP_PRIO_HIGH       = 2,
        CSP_PRIO_RESERVED   = 3,
        CSP_PRIO_NORM       = 4,
        CSP_PRIO_LOW        = 5,
        CSP_PRIO_BULK       = 6,
        CSP_PRIO_DEBUG      = 7
} csp_prio_t;

/** Size of bit-fields in CSP header */
#define CSP_ID_PROTOCOL_SIZE    		3
#define CSP_ID_PRIO_SIZE                3
#define CSP_ID_HOST_SIZE                4
#define CSP_ID_PORT_SIZE                5
#define CSP_ID_FLAGS_SIZE               8

#if CSP_ID_PROTOCOL_SIZE + CSP_ID_PRIO_SIZE + 2 * CSP_ID_HOST_SIZE + 2 * CSP_ID_PORT_SIZE + CSP_ID_FLAGS_SIZE != 32 && __GNUC__
#error "Header lenght must be 32 bits"
#endif

/** Highest number to be entered in field */
#define CSP_ID_PROTOCOL_MAX             ((1 << (CSP_ID_PROTOCOL_SIZE)) - 1)
#define CSP_ID_PRIO_MAX                 ((1 << (CSP_ID_PRIO_SIZE)) - 1)
#define CSP_ID_HOST_MAX                 ((1 << (CSP_ID_HOST_SIZE)) - 1)
#define CSP_ID_PORT_MAX                 ((1 << (CSP_ID_PORT_SIZE)) - 1)
#define CSP_ID_FLAGS_MAX                ((1 << (CSP_ID_FLAGS_SIZE)) - 1)

/** Identifier field masks */
#define CSP_ID_PROTOCOL_MASK    ((uint32_t) CSP_ID_PROTOCOL_MAX << (CSP_ID_FLAGS_SIZE + 2 * CSP_ID_PORT_SIZE + 2 * CSP_ID_HOST_SIZE + CSP_ID_PROTOCOL_SIZE))
#define CSP_ID_PRIO_MASK        ((uint32_t) CSP_ID_PRIO_MAX     << (CSP_ID_FLAGS_SIZE + 2 * CSP_ID_PORT_SIZE + 2 * CSP_ID_HOST_SIZE))
#define CSP_ID_SRC_MASK         ((uint32_t) CSP_ID_HOST_MAX     << (CSP_ID_FLAGS_SIZE + 2 * CSP_ID_PORT_SIZE + 1 * CSP_ID_HOST_SIZE))
#define CSP_ID_DST_MASK         ((uint32_t) CSP_ID_HOST_MAX     << (CSP_ID_FLAGS_SIZE + 2 * CSP_ID_PORT_SIZE))
#define CSP_ID_DPORT_MASK       ((uint32_t) CSP_ID_PORT_MAX     << (CSP_ID_FLAGS_SIZE + 1 * CSP_ID_PORT_SIZE))
#define CSP_ID_SPORT_MASK       ((uint32_t) CSP_ID_PORT_MAX     << (CSP_ID_FLAGS_SIZE))
#define CSP_ID_FLAGS_MASK       ((uint32_t) CSP_ID_FLAGS_MAX    << (0))

#define CSP_ID_CONN_MASK                (CSP_ID_SRC_MASK | CSP_ID_DST_MASK | CSP_ID_DPORT_MASK | CSP_ID_SPORT_MASK)

/** @brief This union defines a CSP identifier and allows to access it in mode standard, extended or through a table. */
typedef union {
	uint32_t ext;
	struct __attribute__((__packed__)) {
#if defined(_CSP_BIG_ENDIAN_) && !defined(_CSP_LITTLE_ENDIAN_)
		unsigned int protocol : CSP_ID_PROTOCOL_SIZE;
		unsigned int pri : CSP_ID_PRIO_SIZE;
		unsigned int src : CSP_ID_HOST_SIZE;
		unsigned int dst : CSP_ID_HOST_SIZE;
		unsigned int dport : CSP_ID_PORT_SIZE;
		unsigned int sport : CSP_ID_PORT_SIZE;
		unsigned int flags : CSP_ID_FLAGS_SIZE;
#elif defined(_CSP_LITTLE_ENDIAN_) && !defined(_CSP_BIG_ENDIAN_)
		unsigned int flags : CSP_ID_FLAGS_SIZE;
		unsigned int sport : CSP_ID_PORT_SIZE;
		unsigned int dport : CSP_ID_PORT_SIZE;
		unsigned int dst : CSP_ID_HOST_SIZE;
		unsigned int src : CSP_ID_HOST_SIZE;
		unsigned int pri : CSP_ID_PRIO_SIZE;
		unsigned int protocol : CSP_ID_PROTOCOL_SIZE;
#else
#error "Must define one of _CSP_BIG_ENDIAN_ or _CSP_LITTLE_ENDIAN_ in csp_platform.h"
#endif
	};
}csp_id_t;

#else

typedef enum {
	CSP_PRIO_CRITICAL	= 0,
	CSP_PRIO_HIGH		= 1,
	CSP_PRIO_NORM 		= 2,
	CSP_PRIO_LOW		= 3,
} csp_prio_t;

/** Size of bit-fields in CSP header */
#define CSP_ID_PRIO_SIZE		2
#define CSP_ID_HOST_SIZE		5
#define CSP_ID_PORT_SIZE		6
#define CSP_ID_FLAGS_SIZE		8

#if CSP_ID_PRIO_SIZE + 2 * CSP_ID_HOST_SIZE + 2 * CSP_ID_PORT_SIZE + CSP_ID_FLAGS_SIZE != 32 && __GNUC__
#error "Header lenght must be 32 bits"
#endif

/** Highest number to be entered in field */
#define CSP_ID_PRIO_MAX			((1 << (CSP_ID_PRIO_SIZE)) - 1)
#define CSP_ID_HOST_MAX			((1 << (CSP_ID_HOST_SIZE)) - 1)
#define CSP_ID_PORT_MAX			((1 << (CSP_ID_PORT_SIZE)) - 1)
#define CSP_ID_FLAGS_MAX		((1 << (CSP_ID_FLAGS_SIZE)) - 1)

/** Identifier field masks */
#define CSP_ID_PRIO_MASK		((uint32_t) CSP_ID_PRIO_MAX 	<< (CSP_ID_FLAGS_SIZE + 2 * CSP_ID_PORT_SIZE + 2 * CSP_ID_HOST_SIZE))
#define CSP_ID_SRC_MASK	 		((uint32_t) CSP_ID_HOST_MAX 	<< (CSP_ID_FLAGS_SIZE + 2 * CSP_ID_PORT_SIZE + 1 * CSP_ID_HOST_SIZE))
#define CSP_ID_DST_MASK	 		((uint32_t) CSP_ID_HOST_MAX 	<< (CSP_ID_FLAGS_SIZE + 2 * CSP_ID_PORT_SIZE))
#define CSP_ID_DPORT_MASK   	((uint32_t) CSP_ID_PORT_MAX 	<< (CSP_ID_FLAGS_SIZE + 1 * CSP_ID_PORT_SIZE))
#define CSP_ID_SPORT_MASK   	((uint32_t) CSP_ID_PORT_MAX 	<< (CSP_ID_FLAGS_SIZE))
#define CSP_ID_FLAGS_MASK		((uint32_t) CSP_ID_FLAGS_MAX 	<< (0))

#define CSP_ID_CONN_MASK		(CSP_ID_SRC_MASK | CSP_ID_DST_MASK | CSP_ID_DPORT_MASK | CSP_ID_SPORT_MASK)

/** @brief This union defines a CSP identifier and allows access to the individual fields or the entire identifier */
typedef union {
	uint32_t ext;
	struct __attribute__((__packed__)) {
#if defined(_CSP_BIG_ENDIAN_) && !defined(_CSP_LITTLE_ENDIAN_)
		unsigned int pri : CSP_ID_PRIO_SIZE;
		unsigned int src : CSP_ID_HOST_SIZE;
		unsigned int dst : CSP_ID_HOST_SIZE;
		unsigned int dport : CSP_ID_PORT_SIZE;
		unsigned int sport : CSP_ID_PORT_SIZE;
		unsigned int flags : CSP_ID_FLAGS_SIZE;
#elif defined(_CSP_LITTLE_ENDIAN_) && !defined(_CSP_BIG_ENDIAN_)
		unsigned int flags : CSP_ID_FLAGS_SIZE;
		unsigned int sport : CSP_ID_PORT_SIZE;
		unsigned int dport : CSP_ID_PORT_SIZE;
		unsigned int dst : CSP_ID_HOST_SIZE;
		unsigned int src : CSP_ID_HOST_SIZE;
		unsigned int pri : CSP_ID_PRIO_SIZE;
#else
#error "Must define one of _CSP_BIG_ENDIAN_ or _CSP_LITTLE_ENDIAN_ in csp_platform.h"
#endif

	};
} csp_id_t;

#endif // CSP_HDR_0.9

/** Broadcast address */
#define CSP_BROADCAST_ADDR	CSP_ID_HOST_MAX

/** Default routing address */
#define CSP_DEFAULT_ROUTE	(CSP_ID_HOST_MAX + 1)

/** CSP Flags */
#define CSP_FRES1			0x80 				// Reserved for future use
#define CSP_FRES2			0x40 				// Reserved for future use
#define CSP_FRES3			0x20 				// Reserved for future use
#define CSP_FRES4			0x10 				// Reserved for future use
#define CSP_FHMAC 			0x08 				// Use HMAC verification
#define CSP_FXTEA 			0x04 				// Use XTEA encryption
#define CSP_FRDP			0x02 				// Use RDP protocol
#define CSP_FCRC32 			0x01 				// Use CRC32 checksum

/** CSP Socket options */
#define CSP_SO_RDPREQ  		0x0001				// Require RDP
#define CSP_SO_RDPPROHIB	0x0002				// Prohibit RDP
#define CSP_SO_HMACREQ 		0x0004				// Require HMAC
#define CSP_SO_HMACPROHIB	0x0008				// Prohibit HMAC
#define CSP_SO_XTEAREQ 		0x0010 				// Require XTEA
#define CSP_SO_XTEAPROHIB	0x0020				// Prohibit HMAC
#define CSP_SO_CRC32REQ		0x0040				// Require CRC32
#define CSP_SO_CRC32PROHIB	0x0080				// Prohibit CRC32

/** CSP Connect options */
#define CSP_O_RDP  			CSP_SO_RDPREQ		// Enable RDP
#define CSP_O_NORDP			CSP_SO_RDPPROHIB	// Disable RDP
#define CSP_O_HMAC 			CSP_SO_HMACREQ		// Enable HMAC
#define CSP_O_NOHMAC		CSP_SO_HMACPROHIB	// Disable HMAC
#define CSP_O_XTEA 			CSP_SO_XTEAREQ		// Enable XTEA
#define CSP_O_NOXTEA		CSP_SO_XTEAPROHIB	// Disable XTEA
#define CSP_O_CRC32			CSP_SO_CRC32REQ		// Enable CRC32
#define CSP_O_NOCRC32		CSP_SO_CRC32PROHIB	// Disable CRC32

/**
 * CSP PACKET STRUCTURE
 * Note: This structure is constructed to fit
 * with all interface frame types in order to 
 * have buffer reuse
 */
typedef struct __attribute__((__packed__)) {
	uint8_t padding[8];		   	// Interface dependent padding
	uint16_t length;			// Length field must be just before CSP ID
	csp_id_t id;				// CSP id must be just before data
	union {
		uint8_t data[0];		// This just points to the rest of the buffer, without a size indication.
		uint16_t data16[0];		// The data 16 and 32 types makes it easy to reference an integer (properly aligned)
		uint32_t data32[0];		// - without the compiler warning about strict aliasing rules.
	};
} csp_packet_t;

/* Next hop function prototype */
typedef int (*nexthop_t)(csp_packet_t * packet, unsigned int timeout);

/* Interface struct */
typedef struct csp_iface_s {
    const char * name;			/**< Interface name */
    nexthop_t nexthop; 			/**< Next hop function */
    uint8_t promisc;			/**< Promiscuous mode enabled */
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
    struct csp_iface_s * next;	/**< Next interface */
} csp_iface_t;

/**
 * This define must be equal to the size of the packet overhead in csp_packet_t
 * it is used in csp_buffer_get() to check the allocated buffer size against
 * the required buffer size.
 */
#define CSP_BUFFER_PACKET_OVERHEAD 	(sizeof(csp_packet_t) - sizeof(((csp_packet_t *) 0)->data))

/** Forward declaration of socket, connection and l4data structure */
typedef struct csp_socket_s csp_socket_t;
typedef struct csp_conn_s csp_conn_t;
typedef struct csp_l4data_s csp_l4data_t;

/* Implemented in csp_io.c */
void csp_init(uint8_t my_node_address);
csp_socket_t * csp_socket(uint32_t opts);
csp_conn_t * csp_accept(csp_socket_t * socket, unsigned int timeout);
csp_packet_t * csp_read(csp_conn_t * conn, unsigned int timeout);
int csp_send(csp_conn_t * conn, csp_packet_t * packet, unsigned int timeout);
int csp_transaction(uint8_t prio, uint8_t dest, uint8_t port, unsigned int timeout, void * outbuf, int outlen, void * inbuf, int inlen);
int csp_transaction_persistent(csp_conn_t * conn, unsigned int timeout, void * outbuf, int outlen, void * inbuf, int inlen);

/* Implemented in csp_conn.c */
csp_conn_t * csp_connect(uint8_t prio, uint8_t dest, uint8_t dport, unsigned int timeout, uint32_t opts);
void csp_close(csp_conn_t * conn);
int csp_conn_dport(csp_conn_t * conn);
int csp_conn_sport(csp_conn_t * conn);
int csp_conn_dst(csp_conn_t * conn);
int csp_conn_src(csp_conn_t * conn);
int csp_conn_flags(csp_conn_t * conn);

/* Implemented in csp_port.c */
int csp_listen(csp_socket_t * socket, size_t conn_queue_length);
int csp_bind(csp_socket_t * socket, uint8_t port);

/* Implemented in csp_route.c */
void csp_route_set(uint8_t node, csp_iface_t * ifc, uint8_t nexthop_mac_addr);
void csp_route_start_task(unsigned int task_stack_size, unsigned int priority);
int csp_promisc_enable(unsigned int buf_size);
csp_packet_t * csp_promisc_read(unsigned int timeout);

/* Implemented in csp_services.c */
void csp_service_handler(csp_conn_t * conn, csp_packet_t * packet);
int csp_ping(uint8_t node, unsigned int timeout, unsigned int size, uint8_t conn_options);
void csp_ping_noreply(uint8_t node);
void csp_ps(uint8_t node, unsigned int timeout);
void csp_memfree(uint8_t node, unsigned int timeout);
void csp_buf_free(uint8_t node, unsigned int timeout);
void csp_reboot(uint8_t node);
void csp_uptime(uint8_t node, unsigned int timeout);

/* Implemented in csp_rdp.c */
void csp_rdp_set_opt(unsigned int window_size, unsigned int conn_timeout_ms, unsigned int packet_timeout_ms, unsigned int delayed_acks, unsigned int ack_timeout, unsigned int ack_delay_count);

/* Key functions */
int csp_xtea_set_key(char * key, uint32_t keylen);
int csp_hmac_set_key(char * key, uint32_t keylen);

/* CSP debug printf - implemented in arch/x/csp_debug.c */
typedef enum {
	CSP_INFO	 = 0,
	CSP_ERROR	= 1,
	CSP_WARN	 = 2,
	CSP_BUFFER   = 3,
	CSP_PACKET   = 4,
	CSP_PROTOCOL = 5,
	CSP_LOCK	 = 6,
} csp_debug_level_t;

#if CSP_DEBUG
typedef void (*csp_debug_hook_func_t)(csp_debug_level_t level, char * str);
void csp_debug(csp_debug_level_t level, const char * format, ...);
void csp_debug_toggle_level(csp_debug_level_t level);
void csp_route_print_interfaces(void);
void csp_route_print_table(void);
void csp_conn_print_table(void);
void csp_buffer_print_table(void);
void csp_debug_hook_set(csp_debug_hook_func_t f);
#else
#define csp_debug(...);
#define csp_debug_toggle_level(...);
#define csp_route_print_interfaces(...);
#define csp_route_print_table(...);
#define csp_conn_print_table(...);
#define csp_buffer_print_table(...);
#define csp_debug_hook_set(...);
#endif

/* Quick and dirty hack to place AVR debug info in progmem */
#if CSP_DEBUG && defined(__AVR__)
#include <avr/pgmspace.h>
#define csp_debug(level, format, ...) printf_P(PSTR(format), ##__VA_ARGS__)
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // _CSP_H_
