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

#include <csp/csp_autoconfig.h>

/* CSP includes */
#include "csp_types.h"
#include "csp_platform.h"
#include "csp_error.h"
#include "csp_debug.h"
#include "csp_buffer.h"
#include "csp_rtable.h"
#include "csp_iflist.h"

/** csp_init
 * Start up the can-space protocol
 * @param my_node_address The CSP node address
 */
int csp_init(uint8_t my_node_address);

/** csp_set_address
 * Set the systems own address
 * @param addr The new address of the system
 */
void csp_set_address(uint8_t addr);

/** csp_get_address
 * Get the systems own address
 * @return The current address of the system
 */
uint8_t csp_get_address(void);

/** csp_set_hostname
 * Set subsystem hostname.
 * This function takes a pointer to a string, which should remain static
 * @param hostname Hostname to set
 */
void csp_set_hostname(const char *hostname);

/** csp_get_hostname
 * Get current subsystem hostname.
 * @return Pointer to char array with current hostname.
 */
const char *csp_get_hostname(void);

/** csp_set_model
 * Set subsystem model name.
 * This function takes a pointer to a string, which should remain static
 * @param model Model name to set
 */
void csp_set_model(const char *model);

/** csp_get_model
 * Get current model name.
 * @return Pointer to char array with current model name.
 */
const char *csp_get_model(void);

/** csp_set_revision
 * Set subsystem revision. This can be used to override the CMP revision field.
 * This function takes a pointer to a string, which should remain static
 * @param revision Revision name to set
 */
void csp_set_revision(const char *revision);

/** csp_get_revision
 * Get subsystem revision.
 * @return Pointer to char array with software revision.
 */
const char *csp_get_revision(void);

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
 * Start the router task.
 * @param task_stack_size The number of portStackType to allocate. This only affects FreeRTOS systems.
 * @param priority The OS task priority of the router
 */
int csp_route_start_task(unsigned int task_stack_size, unsigned int priority);

/**
 * Call the router worker function manually (without the router task)
 * This must be run inside a loop or called periodically for the csp router to work.
 * Use this function instead of calling and starting the router task.
 * @param timeout max blocking time
 * @return -1 if no packet was processed, 0 otherwise
 */
int csp_route_work(uint32_t timeout);

/**
 * Start the bridge task.
 * @param task_stack_size The number of portStackType to allocate. This only affects FreeRTOS systems.
 * @param priority The OS task priority of the router
 * @param _if_a pointer to first side
 * @param _if_b pointer to second side
 * @return CSP_ERR type
 */
int csp_bridge_start(unsigned int task_stack_size, unsigned int task_priority, csp_iface_t * _if_a, csp_iface_t * _if_b);

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
 * Same as csp_sfp_send but with option to supply your own memcpy function.
 * This is usefull if you wish to send data stored in flash memory or another location
 * @param conn pointer to connection
 * @param data pointer to data to send
 * @param totalsize size of data to send
 * @param mtu maximum transfer unit
 * @param timeout timeout in ms to wait for csp_send()
 * @param memcpyfcn, pointer to memcpy function
 * @return 0 if OK, -1 if ERR
 */
int csp_sfp_send_own_memcpy(csp_conn_t * conn, void * data, int totalsize, int mtu, uint32_t timeout, void * (*memcpyfcn)(void *, const void *, size_t));

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
 * This is the counterpart to the csp_sfp_send function
 * @param conn pointer to active conn, on which you expect to receive sfp packed data
 * @param dataout pointer to NULL pointer, whill be overwritten with malloc pointer
 * @param datasize actual size of received data
 * @param timeout timeout in ms to wait for csp_recv()
 * @param first_packet This is a pointer to the first SFP packet (previously received with csp_read)
 * @return 0 if OK, -1 if ERR
 */
int csp_sfp_recv_fp(csp_conn_t * conn, void ** dataout, int * datasize, uint32_t timeout, csp_packet_t * first_packet);

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
 * Shutdown subsystem
 * @param node node id
 */
void csp_shutdown(uint8_t node);

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
 * Print connection table
 */
void csp_conn_print_table(void);
int csp_conn_print_table_str(char * str_buf, int str_size);

/**
 * Print buffer usage table
 */
void csp_buffer_print_table(void);

#ifdef __AVR__
typedef uint32_t csp_memptr_t;
#else
typedef void * csp_memptr_t;
#endif

typedef csp_memptr_t (*csp_memcpy_fnc_t)(csp_memptr_t, const csp_memptr_t, size_t);
void csp_cmp_set_memcpy(csp_memcpy_fnc_t fnc);

/**
 * Set csp_debug hook function
 * @param f Hook function
 */
#include <stdarg.h>
typedef void (*csp_debug_hook_func_t)(csp_debug_level_t level, const char *format, va_list args);
void csp_debug_hook_set(csp_debug_hook_func_t f);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // _CSP_H_
