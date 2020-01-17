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

/**
   @file
   CSP.
*/

#include <csp/csp_platform.h>
#include <csp/csp_debug.h>
#include <csp/csp_buffer.h>
#include <csp/csp_rtable.h>
#include <csp/csp_iflist.h>
#include <csp/csp_sfp.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * CSP configuration.
 * @see csp_init()
 */
typedef struct csp_conf_s {

	uint8_t address;		/**< CSP address of the system */

	const char *hostname;		/**< Host name, returned by the #CSP_CMP_IDENT request */
	const char *model;		/**< Model, returned by the #CSP_CMP_IDENT request */
	const char *revision;		/**< Revision, returned by the #CSP_CMP_IDENT request */

	uint8_t conn_max;		/**< Max number of connections. A fixed connection array is allocated by csp_init() */
	uint8_t conn_queue_length;	/**< Max queue length (max queued Rx messages). */
	uint32_t conn_dfl_so;		/**< Default/minimum connection options. Options will always be or'ed onto new connections, see csp_connect() */
	uint8_t fifo_length;		/**< Length of incoming message queue, used for handover to router task. */
	uint8_t port_max_bind;		/**< Max/highest port for use with csp_bind() */
	uint8_t rdp_max_window;		/**< Max RDP window size */

} csp_conf_t;

/**
 * Get default CSP configuration.
 */
static inline void csp_conf_get_defaults(csp_conf_t * conf) {
	conf->address = 1;
	conf->hostname = "hostname";
	conf->model = "model";
	conf->revision = "resvision";
	conf->conn_max = 10;
	conf->conn_queue_length = 10;
	conf->conn_dfl_so = CSP_O_NONE;
	conf->fifo_length = 25;
	conf->port_max_bind = 24;
	conf->rdp_max_window = 20;
}

/**
 * Initialize CSP.
 * This will configure/allocate basic structures.
 * @param[in] conf configuration. A shallow copy will be done of the configuration, i.e. only copy references to strings/structers.
 * @return #CSP_ERR_NONE on success, otherwise an error code.
 */
int csp_init(const csp_conf_t * conf);

/**
 * Free allocated resorces in CSP (for testing).
 * This is only to be called from automatic tests, to satisfy tools like valgrind.
 */
void csp_free_resources(void);

/**
 * Get a \a read-only reference to the active CSP configuration.
 * @return Active CSP configuration (read-only).
 */
const csp_conf_t * csp_get_conf(void);

/**
 * Get the systems own address.
 * @return The current address of the system
 */
uint8_t csp_get_address(void);

/**
 * Create a CSP socket endpoint.
 * @param[in] opts Socket options.
 * @return Socket on success, NULL on failure
 */
csp_socket_t *csp_socket(uint32_t opts);

/**
 * Wait for a new connection on a socket created by csp_socket
 * @param socket Socket to accept connections on
 * @param timeout timeout in mS, use #CSP_MAX_TIMEOUT for infinite timeout.
 * @return New connection on success, NULL on failure or timeout.
 */
csp_conn_t *csp_accept(csp_socket_t *socket, uint32_t timeout);

/**
 * Read data from a connection
 * This fuction uses the RX queue of a connection to receive a packet
 * If no packet is available and a timeout has been specified
 * The call will block.
 * Do NOT call this from ISR
 * @param conn pointer to connection
 * @param timeout timeout in mS, use #CSP_MAX_TIMEOUT for infinite timeout.
 * @return Packet or NULL in case of failure or timeout.
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
 * @param opts Connection options.
 * @return Return 1 or reply size if successful, 0 if error or incoming length does not match or -1 if timeout was reached
 */
int csp_transaction_w_opts(uint8_t prio, uint8_t dest, uint8_t port, uint32_t timeout, void *outbuf, int outlen, void *inbuf, int inlen, uint32_t opts);

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
static inline int csp_transaction(uint8_t prio, uint8_t dest, uint8_t port, uint32_t timeout, void * outbuf, int outlen, void * inbuf, int inlen) {
	return csp_transaction_w_opts(prio, dest, port, timeout, outbuf, outlen, inbuf, inlen, 0);
}

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
 * @param socket connection-less socket.
 * @param timeout timeout in mS, use #CSP_MAX_TIMEOUT for infinite timeout.
 * @return Packet or NULL in case of failure or timeout.
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
 * @return #CSP_ERR_NONE on success, otherwise an error code and the packet must be freed.
 */
int csp_sendto(uint8_t prio, uint8_t dest, uint8_t dport, uint8_t src_port, uint32_t opts, csp_packet_t *packet, uint32_t timeout);

/**
 * Send a packet as a direct reply to the source of an incoming packet,
 * but still without holding an entire connection
 * @param request_packet pointer to packet to reply to
 * @param reply_packet actual reply data
 * @param opts CSP_O_x
 * @param timeout timeout used by interfaces with blocking send
 * @return #CSP_ERR_NONE on success, otherwise an error code and the reply_packet must be freed.
 */
int csp_sendto_reply(const csp_packet_t * request_packet, csp_packet_t * reply_packet, uint32_t opts, uint32_t timeout);

/**
 * Used to establish outgoing connections
 * This function searches the port table for free slots and finds an unused
 * connection from the connection pool
 * There is no handshake in the CSP protocol
 * @param prio Connection priority.
 * @param dest Destination address.
 * @param dport Destination port.
 * @param timeout Timeout in ms.
 * @param opts Connection options.
 * @return New connectio or NULL on failure.
 */
csp_conn_t *csp_connect(uint8_t prio, uint8_t dest, uint8_t dport, uint32_t timeout, uint32_t opts);

/**
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
 * @param backlog Lenght of backlog connection queue. Queue holds incoming connections and returned by csp_accept().
 * @return #CSP_ERR_NONE on success, otherwise an error code.
 */
int csp_listen(csp_socket_t *socket, size_t backlog);

/**
 * Bind port to socket
 * @param socket Socket to bind port to
 * @param port Port number to bind
 * @return #CSP_ERR_NONE on success, otherwise an error code.
 */
int csp_bind(csp_socket_t *socket, uint8_t port);

/**
 * Start the router task.
 * @param task_stack_size The number of portStackType to allocate. This only affects FreeRTOS systems.
 * @param task_priority The OS task priority of the router
 * @return #CSP_ERR_NONE on success, otherwise an error code.
 */
int csp_route_start_task(unsigned int task_stack_size, unsigned int task_priority);

/**
 * Call the router worker function manually (without the router task)
 * This must be run inside a loop or called periodically for the csp router to work.
 * Use this function instead of calling and starting the router task.
 * @param timeout max blocking time
 * @return #CSP_ERR_NONE on success, otherwise an error code.
 */
int csp_route_work(uint32_t timeout);

/**
 * Start the bridge task.
 * @param task_stack_size The number of portStackType to allocate. This only affects FreeRTOS systems.
 * @param task_priority The OS task priority of the router
 * @param _if_a pointer to first side
 * @param _if_b pointer to second side
 * @return #CSP_ERR_NONE on success, otherwise an error code.
 */
int csp_bridge_start(unsigned int task_stack_size, unsigned int task_priority, csp_iface_t * _if_a, csp_iface_t * _if_b);

/**
 * Enable promiscuous mode packet queue
 * This function is used to enable promiscuous mode for the router.
 * If enabled, a copy of all incoming packets are placed in a queue
 * that can be read with csp_promisc_read(). Not all interface drivers
 * support promiscuous mode.
 *
 * @param queue_size Size (max length) of queue for incoming packets
 * @return #CSP_ERR_NONE on success, otherwise an error code.
 */
int csp_promisc_enable(unsigned int queue_size);

/**
 * Disable promiscuous mode.
 * If the queue was initialised prior to this, it can be re-enabled
 * by calling promisc_enable(0)
 */
void csp_promisc_disable(void);

/**
 * Get packet from promiscuous packet queue
 * Returns the first packet from the promiscuous packet queue.
 * The queue is FIFO, so the returned packet is the oldest one
 * in the queue.
 *
 * @param timeout Timeout in ms to wait for a new packet
 * @return Packet (free with csp_buffer_free() or re-use packet), NULL on error or timeout.
 */
csp_packet_t *csp_promisc_read(uint32_t timeout);

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
 * Request subsystem free memory.
 * @param node address of subsystem.
 * @param timeout timeout in ms
 * @param[out] size free memory.
 * @return #CSP_ERR_NONE on success, otherwise an error code.
 */
int csp_get_memfree(uint8_t node, uint32_t timeout, uint32_t * size);

/**
 * Request subsystem free memory and print to stdout.
 * @param node address of subsystem.
 * @param timeout timeout in ms
 */
void csp_memfree(uint8_t node, uint32_t timeout);

/**
 * Request subsystem free buffers.
 * @param[in] node address of subsystem.
 * @param[in] timeout timeout in ms
 * @param[out] size free buffers.
 * @return #CSP_ERR_NONE on success, otherwise an error code.
 */
int csp_get_buf_free(uint8_t node, uint32_t timeout, uint32_t * size);

/**
 * Request subsystem free buffers and print to stdout.
 * @param node address of subsystem.
 * @param timeout timeout in ms
 */
void csp_buf_free(uint8_t node, uint32_t timeout);

/**
 * Reboot subsystem.
 * @param node address of subsystem.
 */
void csp_reboot(uint8_t node);

/**
 * Shutdown subsystem.
 * @param node address of subsystem.
 */
void csp_shutdown(uint8_t node);

/**
 * Request subsystem uptime and print to stdout.
 * @param node address of subsystem.
 * @param timeout timeout in ms
 */
void csp_uptime(uint8_t node, uint32_t timeout);

/**
 * Request subsystem uptime
 * @param[in] node address of subsystem.
 * @param[in] timeout timeout in ms
 * @param[out] uptime uptime in seconds.
 * @return #CSP_ERR_NONE on success, otherwise an error code.
 */
int csp_get_uptime(uint8_t node, uint32_t timeout, uint32_t * uptime);

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
 * Print connection table to stdout.
 */
void csp_conn_print_table(void);

/**
 * Print connection table to string.
 */
int csp_conn_print_table_str(char * str_buf, int str_size);

/**
 * Print buffer usage table to stdout.
 */
void csp_buffer_print_table(void);

/**
 * Hex dump to stdout
 */
void csp_hex_dump(const char *desc, void *addr, int len);

/**
   Set platform specific memory copy function.
*/
void csp_cmp_set_memcpy(csp_memcpy_fnc_t fnc);

#ifdef __cplusplus
}
#endif
#endif
