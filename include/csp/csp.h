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

/**
 * RESERVED PORTS (SERVICES)
 */

enum csp_reserved_ports_e {
	CSP_PING			= 1,
	CSP_PS			  	= 2,
	CSP_MEMFREE		 	= 3,
	CSP_REBOOT		  	= 4,
	CSP_BUF_FREE		= 5,
	CSP_ANY			 	= (CSP_MAX_BIND_PORT + 1),
	CSP_PROMISC		 	= (CSP_MAX_BIND_PORT + 2)
};

/**
 * PRIORITIES
 */

typedef enum csp_prio_e {
	CSP_PRIO_CRITICAL	= 0,
	CSP_PRIO_HIGH		= 1,
	CSP_PRIO_NORM 		= 2,
	CSP_PRIO_LOW		= 3,
} csp_prio_t;

/**
 * CSP Protocol Types
 */

/** The address of the node */
extern uint8_t my_address;

/** Define used to specify MAC_ADDR = NODE_ID */
#define CSP_NODE_MAC			0xFF

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

	unsigned int pri		: CSP_ID_PRIO_SIZE;
	unsigned int src		: CSP_ID_HOST_SIZE;
	unsigned int dst		: CSP_ID_HOST_SIZE;
	unsigned int dport		: CSP_ID_PORT_SIZE;
	unsigned int sport		: CSP_ID_PORT_SIZE;
	unsigned int flags		: CSP_ID_FLAGS_SIZE;

#elif defined(_CSP_LITTLE_ENDIAN_) && !defined(_CSP_BIG_ENDIAN_)

	unsigned int flags		: CSP_ID_FLAGS_SIZE;
	unsigned int sport		: CSP_ID_PORT_SIZE;
	unsigned int dport		: CSP_ID_PORT_SIZE;
	unsigned int dst		: CSP_ID_HOST_SIZE;
	unsigned int src		: CSP_ID_HOST_SIZE;
	unsigned int pri		: CSP_ID_PRIO_SIZE;

#else

  #error "Must define one of _CSP_BIG_ENDIAN_ or _CSP_LITTLE_ENDIAN_ in csp_platform.h"

#endif

  };
} csp_id_t;

/** Broadcast address */
#define CSP_BROADCAST_ADDR	CSP_ID_HOST_MAX

/** Default routing address */
#define CSP_DEFAULT_ROUTE	(CSP_ID_HOST_MAX + 1)

/** CSP Flags */
#define CSP_FRES1				0x80 // Reserved for future use
#define CSP_FRES2				0x40 // Reserved for future use
#define CSP_FRES3				0x20 // Reserved for future use
#define CSP_FRES4				0x10 // Reserved for future use
#define CSP_FHMAC 				0x08 // Use HMAC verification/generation
#define CSP_FXTEA 				0x04 // Use XTEA encryption/decryption
#define CSP_FRDP				0x02 // Use RDP protocol
#define CSP_FCRC 				0x01 // Use CRC32 checksum (Not implemented)

/** CSP Socket options */
#define CSP_SO_RDPREQ  			0x0001
#define CSP_SO_HMACREQ 			0x0002
#define CSP_SO_XTEAREQ 			0x0004

/** CSP Connect options */
#define CSP_O_RDP  				CSP_SO_RDPREQ
#define CSP_O_HMAC 				CSP_SO_HMACREQ
#define CSP_O_XTEA 				CSP_SO_XTEAREQ

/**
 * CSP PACKET STRUCTURE
 * Note: This structure is constructed to fit
 * with all interface frame types in order to 
 * have buffer reuse
 */
typedef struct __attribute__((__packed__)) {
	uint8_t padding[44];	   	// Interface dependent padding
	uint16_t length;			// Length field must be just before CSP ID
	csp_id_t id;				// CSP id must be just before data
	union {
		uint8_t data[0];		// This just points to the rest of the buffer, without a size indication.
		uint16_t data16[0];		// The data 16 and 32 types makes it easy to reference an integer (properly aligned)
		uint32_t data32[0];		// - without the compiler warning about strict aliasing rules.
	};
} csp_packet_t;

/**
 * This define must be equal to the size of the packet overhead in csp_packet_t
 * it is used in csp_buffer_get() to check the allocated buffer size against
 * the required buffer size.
 */
#define CSP_BUFFER_PACKET_OVERHEAD 	(44 + sizeof(uint16_t) + sizeof(csp_id_t))

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

/* Implemented in csp_port.c */
int csp_listen(csp_socket_t * socket, size_t conn_queue_length);
int csp_bind(csp_socket_t * socket, uint8_t port);

/* Implemented in csp_route.c */
typedef int (*nexthop_t)(csp_id_t idout, csp_packet_t * packet, unsigned int timeout);
void csp_route_set(const char * name, uint8_t node, nexthop_t nexthop, uint8_t nexthop_mac_addr);
void csp_route_start_task(unsigned int task_stack_size, unsigned int priority);
int csp_promisc_enable(unsigned int buf_size);
csp_packet_t * csp_promisc_read(unsigned int timeout);

/* Implemented in csp_services.c */
void csp_service_handler(csp_conn_t * conn, csp_packet_t * packet);
void csp_ping(uint8_t node, unsigned int timeout);
void csp_ping_noreply(uint8_t node);
void csp_ps(uint8_t node, unsigned int timeout);
void csp_memfree(uint8_t node, unsigned int timeout);
void csp_buf_free(uint8_t node, unsigned int timeout);
void csp_reboot(uint8_t node);

/* Implemented in csp_rdp.c */
void csp_rdp_set_opt(unsigned int window_size, unsigned int conn_timeout_ms, unsigned int packet_timeout_ms);

/* CSP debug printf - implemented in arch/x/csp_debug.c */
typedef enum csp_debug_level_e {
	CSP_INFO	 = 0,
	CSP_ERROR	= 1,
	CSP_WARN	 = 2,
	CSP_BUFFER   = 3,
	CSP_PACKET   = 4,
	CSP_PROTOCOL = 5,
	CSP_LOCK	 = 6,
} csp_debug_level_t;

#if CSP_DEBUG
void csp_debug(csp_debug_level_t level, const char * format, ...);
void csp_debug_toggle_level(csp_debug_level_t level);
void csp_route_print_table(void);
void csp_conn_print_table(void);
void csp_buffer_print_table(void);
#else
#define csp_debug(...);
#define csp_debug_toggle_level(...);
#define csp_route_print(...);
#define csp_conn_print_table(...);
#define csp_buffer_print_table(...);
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // _CSP_H_
