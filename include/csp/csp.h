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

#ifndef _CSP_H_
#define _CSP_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes */
#include <stdint.h>

/* Include configuration file */
#include <csp/csp_autoconfig.h>

/* Make bool for compilers without stdbool.h */
#ifdef CSP_HAVE_STDBOOL_H
#include <stdbool.h>
#else
#define bool int
#define false 0
#define true !false
#endif

/* CSP includes */
#include "csp_platform.h"
#include "csp_error.h"
#include "csp_debug.h"
#include "csp_buffer.h"

/** The address of the node */
extern uint8_t my_address;

/** Define used to specify MAC_ADDR = NODE_ID */
#define CSP_NODE_MAC	0xFF

/**
 * RESERVED PORTS (SERVICES)
 */

enum csp_reserved_ports_e {
	CSP_CMP				= 0,
	CSP_PING			= 1,
	CSP_PS			  	= 2,
	CSP_MEMFREE		 	= 3,
	CSP_REBOOT		  	= 4,
	CSP_BUF_FREE		= 5,
	CSP_UPTIME			= 6,
	CSP_ANY			 	= (CSP_MAX_BIND_PORT + 1),
	CSP_PROMISC		 	= (CSP_MAX_BIND_PORT + 2)
};

typedef enum {
	CSP_PRIO_CRITICAL	= 0,
	CSP_PRIO_HIGH		= 1,
	CSP_PRIO_NORM 		= 2,
	CSP_PRIO_LOW		= 3,
} csp_prio_t;

#define CSP_PRIORITIES			(1 << CSP_ID_PRIO_SIZE)

#ifdef CSP_USE_QOS
#define CSP_RX_QUEUE_LENGTH		(CSP_CONN_QUEUE_LENGTH / CSP_PRIORITIES)
#define CSP_ROUTE_FIFOS			CSP_PRIORITIES
#define CSP_RX_QUEUES			CSP_PRIORITIES
#else
#define CSP_RX_QUEUE_LENGTH 	CSP_CONN_QUEUE_LENGTH
#define CSP_ROUTE_FIFOS			1
#define CSP_RX_QUEUES			1
#endif

/** Size of bit-fields in CSP header */
#define CSP_ID_PRIO_SIZE		2
#define CSP_ID_HOST_SIZE		5
#define CSP_ID_PORT_SIZE		6
#define CSP_ID_FLAGS_SIZE		8

#define CSP_HEADER_BITS			(CSP_ID_PRIO_SIZE + 2 * CSP_ID_HOST_SIZE + 2 * CSP_ID_PORT_SIZE + CSP_ID_FLAGS_SIZE)
#define CSP_HEADER_LENGTH		(CSP_HEADER_BITS/8)

#if CSP_HEADER_BITS != 32 && __GNUC__
#error "Header length must be 32 bits"
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
#define CSP_BROADCAST_ADDR	CSP_ID_HOST_MAX

/** Default routing address */
#define CSP_DEFAULT_ROUTE	(CSP_ID_HOST_MAX + 1)

/** CSP Flags */
#define CSP_FRES1			0x80 				// Reserved for future use
#define CSP_FRES2			0x40 				// Reserved for future use
#define CSP_FRES3			0x20 				// Reserved for future use
#define CSP_FFRAG			0x10 				// Use fragmentation
#define CSP_FHMAC 			0x08 				// Use HMAC verification
#define CSP_FXTEA 			0x04 				// Use XTEA encryption
#define CSP_FRDP			0x02 				// Use RDP protocol
#define CSP_FCRC32 			0x01 				// Use CRC32 checksum

/** CSP Socket options */
#define CSP_SO_NONE 		0x0000				// No socket options
#define CSP_SO_RDPREQ  		0x0001				// Require RDP
#define CSP_SO_RDPPROHIB	0x0002				// Prohibit RDP
#define CSP_SO_HMACREQ 		0x0004				// Require HMAC
#define CSP_SO_HMACPROHIB	0x0008				// Prohibit HMAC
#define CSP_SO_XTEAREQ 		0x0010				// Require XTEA
#define CSP_SO_XTEAPROHIB	0x0020				// Prohibit HMAC
#define CSP_SO_CRC32REQ		0x0040				// Require CRC32
#define CSP_SO_CRC32PROHIB	0x0080				// Prohibit CRC32
#define CSP_SO_CONN_LESS	0x0100				// Enable Connection Less mode

/** CSP Connect options */
#define CSP_O_NONE  		CSP_SO_NONE			// No connection options
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
	uint8_t padding[CSP_PADDING_BYTES];	/**< Interface dependent padding */
	uint16_t length;					/**< Length field must be just before CSP ID */
	csp_id_t id;						/**< CSP id must be just before data */
	union {
		uint8_t data[0];				/**< This just points to the rest of the buffer, without a size indication. */
		uint16_t data16[0];				/**< The data 16 and 32 types makes it easy to reference an integer (properly aligned) */
		uint32_t data32[0];				/**< without the compiler warning about strict aliasing rules. */
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
	uint8_t promisc;			/**< Promiscuous mode enabled */
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

/** csp_init
 * Start up the can-space protocol
 * @param my_node_address The CSP node address
 */
int csp_init(uint8_t my_node_address);

#define CSP_HOSTNAME_LEN	20
/** csp_set_hostname
 * Set subsystem hostname.
 * This function takes a pointer to a string, which should remain static
 * @param hostname Hostname to set
 */
void csp_set_hostname(char *hostname);

/** csp_get_hostname
 * Get current subsystem hostname.
 * @return Pointer to char array with current hostname.
 */
char *csp_get_hostname(void);

#define CSP_MODEL_LEN		30
/** csp_set_model
 * Set subsystem model name.
 * This function takes a pointer to a string, which should remain static
 * @param model Model name to set
 */
void csp_set_model(char *model);

/** csp_get_model
 * Get current model name.
 * @return Pointer to char array with current model name.
 */
char *csp_get_model(void);

/** csp_socket
 * Create CSP socket endpoint
 * @param opts Socket options
 * @return Pointer to socket on success, NULL on failure
 */
csp_socket_t *csp_socket(uint32_t opts);

/**
 * Wait for a new connection on a socket created by csp_socket
 * @param socket Socket to accept connections on
 * @param timeout use CSP_MAX_DELAY for infinite timeout
 * @return Return pointer to csp_conn_t or NULL if timeout was reached
 */
csp_conn_t *csp_accept(csp_socket_t *socket, uint32_t timeout);

/**
 * Read data from a connection
 * This fuction uses the RX queue of a connection to receive a packet
 * If no packet is available and a timeout has been specified
 * The call will block.
 * Do NOT call this from ISR
 * @param conn pointer to connection
 * @param timeout timeout in ms, use CSP_MAX_DELAY for infinite blocking time
 * @return Returns pointer to csp_packet_t, which you MUST free yourself, either by calling csp_buffer_free() or reusing the buffer for a new csp_send.
 */
csp_packet_t *csp_read(csp_conn_t *conn, uint32_t timeout);

/**
 * Send a packet on an already established connection
 * @param conn pointer to connection
 * @param packet pointer to packet,
 * @param timeout a timeout to wait for TX to complete. NOTE: not all underlying drivers supports flow-control.
 * @return returns 1 if successful and 0 otherwise. you MUST free the frame yourself if the transmission was not successful.
 */
int csp_send(csp_conn_t *conn, csp_packet_t *packet, uint32_t timeout);

/**
 * Send a packet on an already established connection, and change the default priority of the connection
 *
 * @note When using this function, the priority of the connection will change. If you need to change it back
 * use another call to csp_send_prio, or ensure that all packets sent on a given connection is using send_prio call.
 *
 * @param prio csp priority
 * @param conn pointer to connection
 * @param packet pointer to packet,
 * @param timeout a timeout to wait for TX to complete. NOTE: not all underlying drivers supports flow-control.
 * @return returns 1 if successful and 0 otherwise. you MUST free the frame yourself if the transmission was not successful.
 */
int csp_send_prio(uint8_t prio, csp_conn_t *conn, csp_packet_t *packet, uint32_t timeout);

/**
 * Perform an entire request/reply transaction
 * Copies both input buffer and reply to output buffeer.
 * Also makes the connection and closes it again
 * @param prio CSP Prio
 * @param dest CSP Dest
 * @param port CSP Port
 * @param timeout timeout in ms
 * @param outbuf pointer to outgoing data buffer
 * @param outlen length of request to send
 * @param inbuf pointer to incoming data buffer
 * @param inlen length of expected reply, -1 for unknown size (note inbuf MUST be large enough)
 * @return Return 1 or reply size if successful, 0 if error or incoming length does not match or -1 if timeout was reached
 */
int csp_transaction(uint8_t prio, uint8_t dest, uint8_t port, uint32_t timeout, void *outbuf, int outlen, void *inbuf, int inlen);

/**
 * Use an existing connection to perform a transaction,
 * This is only possible if the next packet is on the same port and destination!
 * @param conn pointer to connection structure
 * @param timeout timeout in ms
 * @param outbuf pointer to outgoing data buffer
 * @param outlen length of request to send
 * @param inbuf pointer to incoming data buffer
 * @param inlen length of expected reply, -1 for unknown size (note inbuf MUST be large enough)
 * @return
 */
int csp_transaction_persistent(csp_conn_t *conn, uint32_t timeout, void *outbuf, int outlen, void *inbuf, int inlen);

/**
 * Read data from a connection-less server socket
 * This fuction uses the socket directly to receive a frame
 * If no packet is available and a timeout has been specified the call will block.
 * Do NOT call this from ISR
 * @return Returns pointer to csp_packet_t, which you MUST free yourself, either by calling csp_buffer_free() or reusing the buffer for a new csp_send.
 */
csp_packet_t *csp_recvfrom(csp_socket_t *socket, uint32_t timeout);

/**
 * Send a packet without previously opening a connection
 * @param prio CSP_PRIO_x
 * @param dest destination node
 * @param dport destination port
 * @param src_port source port
 * @param opts CSP_O_x
 * @param packet pointer to packet
 * @param timeout timeout used by interfaces with blocking send
 * @return -1 if error (you must free packet), 0 if OK (you must discard pointer)
 */
int csp_sendto(uint8_t prio, uint8_t dest, uint8_t dport, uint8_t src_port, uint32_t opts, csp_packet_t *packet, uint32_t timeout);

/**
 * Send a packet as a direct reply to the source of an incoming packet,
 * but still without holding an entire connection
 * @param request_packet pointer to packet to reply to
 * @param reply_packet actual reply data
 * @param opts CSP_O_x
 * @param timeout timeout used by interfaces with blocking send
 * @return -1 if error (you must free packet), 0 if OK (you must discard pointer)
 */
int csp_sendto_reply(csp_packet_t * request_packet, csp_packet_t * reply_packet, uint32_t opts, uint32_t timeout);

/** csp_connect
 * Used to establish outgoing connections
 * This function searches the port table for free slots and finds an unused
 * connection from the connection pool
 * There is no handshake in the CSP protocol
 * @param prio Connection priority.
 * @param dest Destination address.
 * @param dport Destination port.
 * @param timeout Timeout in ms.
 * @param opts Connection options.
 * @return a pointer to a new connection or NULL
 */
csp_conn_t *csp_connect(uint8_t prio, uint8_t dest, uint8_t dport, uint32_t timeout, uint32_t opts);

/** csp_close
 * Closes a given connection and frees buffers used.
 * @param conn pointer to connection structure
 * @return CSP_ERR_NONE if connection was closed. Otherwise, an err code is returned.
 */
int csp_close(csp_conn_t *conn);

/**
 * @param conn pointer to connection structure
 * @return destination port of an incoming connection
 */
int csp_conn_dport(csp_conn_t *conn);

/**
 * @param conn pointer to connection structure
 * @return source port of an incoming connection
 */
int csp_conn_sport(csp_conn_t *conn);

/**
 * @param conn pointer to connection structure
 * @return destination address of an incoming connection
 */
int csp_conn_dst(csp_conn_t *conn);

/**
 * @param conn pointer to connection structure
 * @return source address of an incoming connection
 */
int csp_conn_src(csp_conn_t *conn);

/**
 * @param conn pointer to connection structure
 * @return flags field of an incoming connection
 */
int csp_conn_flags(csp_conn_t *conn);

/**
 * Set socket to listen for incoming connections
 * @param socket Socket to enable listening on
 * @param conn_queue_length Lenght of backlog connection queue
 * @return 0 on success, -1 on error.
 */
int csp_listen(csp_socket_t *socket, size_t conn_queue_length);

/**
 * Bind port to socket
 * @param socket Socket to bind port to
 * @param port Port number to bind
 * @return 0 on success, -1 on error.
 */
int csp_bind(csp_socket_t *socket, uint8_t port);

/**
 * Set route
 * This function maintains the routing table,
 * To set default route use nodeid CSP_DEFAULT_ROUTE
 * To clear a value pass a NULL value
 */
int csp_route_set(uint8_t node, csp_iface_t *ifc, uint8_t nexthop_mac_addr);

#define CSP_ROUTE_COUNT 			(CSP_ID_HOST_MAX + 2)
#define CSP_ROUTE_TABLE_SIZE		5 * CSP_ROUTE_COUNT

/**
 * Clear and init the routing table
 *
 * This function is called by csp_init and should only be needed in case
 * the table must be reset after initialization.
 * @return CSP_ERR
 */
int csp_route_table_init(void);

/**
 * Load the routing table from a buffer
 *
 * Warning:
 * The table will be RAW from memory and contains direct pointers, not interface names.
 * Therefore it's very important that a saved routing table is deleted after a firmware update
 *
 * @param route_table_in pointer to routing table buffer
 */
void csp_route_table_load(uint8_t route_table_in[CSP_ROUTE_TABLE_SIZE]);

/**
 * Save the routing table to a buffer
 *
 * Warning:
 * The table will be RAW from memory and contains direct pointers, not interface names.
 * Therefore it's very important that a saved routing table is deleted after a firmware update
 *
 * @param route_table_out pointer to routing table buffer
 */
void csp_route_table_save(uint8_t route_table_out[CSP_ROUTE_TABLE_SIZE]);

/**
 * Start the router task.
 * @param task_stack_size The number of portStackType to allocate. This only affects FreeRTOS systems.
 * @param priority The OS task priority of the router
 */
int csp_route_start_task(unsigned int task_stack_size, unsigned int priority);

/**
 * Enable promiscuous mode packet queue
 * This function is used to enable promiscuous mode for the router.
 * If enabled, a copy of all incoming packets are placed in a queue
 * that can be read with csp_promisc_get(). Not all interface drivers
 * support promiscuous mode.
 *
 * @param buf_size Size of buffer for incoming packets
 */
int csp_promisc_enable(unsigned int buf_size);

/**
 * Disable promiscuous mode.
 * If the queue was initialised prior to this, it can be re-enabled
 * by calling promisc_enable(0)
 */
void csp_promisc_disable(void);

/**
 * Get packet from promiscuous mode packet queue
 * Returns the first packet from the promiscuous mode packet queue.
 * The queue is FIFO, so the returned packet is the oldest one
 * in the queue.
 *
 * @param timeout Timeout in ms to wait for a new packet
 */
csp_packet_t *csp_promisc_read(uint32_t timeout);

/**
 * Send multiple packets using the simple fragmentation protocol
 * CSP will add total size and offset to all packets
 * This can be read by the client using the csp_sfp_recv, if the CSP_FFRAG flag is set
 * @param conn pointer to connection
 * @param data pointer to data to send
 * @param totalsize size of data to send
 * @param mtu maximum transfer unit
 * @param timeout timeout in ms to wait for csp_send()
 * @return 0 if OK, -1 if ERR
 */
int csp_sfp_send(csp_conn_t * conn, void * data, int totalsize, int mtu, uint32_t timeout);

/**
 * This is the counterpart to the csp_sfp_send function
 * @param conn pointer to active conn, on which you expect to receive sfp packed data
 * @param dataout pointer to NULL pointer, whill be overwritten with malloc pointer
 * @param datasize actual size of received data
 * @param timeout timeout in ms to wait for csp_recv()
 * @return 0 if OK, -1 if ERR
 */
int csp_sfp_recv(csp_conn_t * conn, void ** dataout, int * datasize, uint32_t timeout);

/**
 * If the given packet is a service-request (that is uses one of the csp service ports)
 * it will be handled according to the CSP service handler.
 * This function will either use the packet buffer or delete it,
 * so this function is typically called in the last "default" clause of
 * a switch/case statement in a csp_listener task.
 * In order to listen to csp service ports, bind your listener to the CSP_ANY port.
 * This function may only be called from task context.
 * @param conn Pointer to the new connection
 * @param packet Pointer to the first packet, obtained by using csp_read()
 */
void csp_service_handler(csp_conn_t *conn, csp_packet_t *packet);

/**
 * Send a single ping/echo packet
 * @param node node id
 * @param timeout timeout in ms
 * @param size size of packet to transmit
 * @param conn_options csp connection options
 * @return >0 = Echo time in ms, -1 = ERR
 */
int csp_ping(uint8_t node, uint32_t timeout, unsigned int size, uint8_t conn_options);

/**
 * Send a single ping/echo packet without waiting for reply
 * @param node node id
 */
void csp_ping_noreply(uint8_t node);

/**
 * Request process list.
 * @note This is only available for FreeRTOS systems
 * @param node node id
 * @param timeout timeout in ms
 */
void csp_ps(uint8_t node, uint32_t timeout);

/**
 * Request amount of free memory
 * @param node node id
 * @param timeout timeout in ms
 */
void csp_memfree(uint8_t node, uint32_t timeout);

/**
 * Request number of free buffer elements
 * @param node node id
 * @param timeout timeout in ms
 */
void csp_buf_free(uint8_t node, uint32_t timeout);

/**
 * Reboot subsystem
 * @param node node id
 */
void csp_reboot(uint8_t node);

/**
 * Request subsystem uptime
 * @param node node id
 * @param timeout timeout in ms
 */
void csp_uptime(uint8_t node, uint32_t timeout);

/**
 * Set RDP options
 * @param window_size Window size
 * @param conn_timeout_ms Connection timeout in ms
 * @param packet_timeout_ms Packet timeout in ms
 * @param delayed_acks Enable/disable delayed acknowledgements
 * @param ack_timeout Acknowledgement timeout when delayed ACKs is enabled
 * @param ack_delay_count Send acknowledgement for every ack_delay_count packets
 */
void csp_rdp_set_opt(unsigned int window_size, unsigned int conn_timeout_ms,
		unsigned int packet_timeout_ms, unsigned int delayed_acks,
		unsigned int ack_timeout, unsigned int ack_delay_count);

/**
 * Get RDP options
 * @param window_size Window size
 * @param conn_timeout_ms Connection timeout in ms
 * @param packet_timeout_ms Packet timeout in ms
 * @param delayed_acks Enable/disable delayed acknowledgements
 * @param ack_timeout Acknowledgement timeout when delayed ACKs is enabled
 * @param ack_delay_count Send acknowledgement for every ack_delay_count packets
 */
void csp_rdp_get_opt(unsigned int *window_size, unsigned int *conn_timeout_ms,
		unsigned int *packet_timeout_ms, unsigned int *delayed_acks,
		unsigned int *ack_timeout, unsigned int *ack_delay_count);

/**
 * Set XTEA key
 * @param key Pointer to key array
 * @param keylen Length of key
 * @return 0 if key was successfully set, -1 otherwise
 */
int csp_xtea_set_key(char *key, uint32_t keylen);

/**
 * Set HMAC key
 * @param key Pointer to key array
 * @param keylen Length of key
 * @return 0 if key was successfully set, -1 otherwise
 */
int csp_hmac_set_key(char *key, uint32_t keylen);

/**
 * Print interface statistics
 */
void csp_route_print_interfaces(void);

/**
 * Print routing table
 */
void csp_route_print_table(void);

/**
 * Print connection table
 */
void csp_conn_print_table(void);

/**
 * Print buffer usage table
 */
void csp_buffer_print_table(void);

/**
 * Set csp_debug hook function
 * @param f Hook function
 */
typedef void (*csp_debug_hook_func_t)(csp_debug_level_t level, char *str);
void csp_debug_hook_set(csp_debug_hook_func_t f);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // _CSP_H_
