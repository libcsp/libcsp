/*****************************************************************************
 * File: csp.h
 * Description: CSP Library Header File
 ****************************************************************************/
#pragma once

#include <csp/csp_error.h>
#include <csp/csp_debug.h>
#include <csp/csp_buffer.h>
#include <csp/csp_rtable.h>
#include <csp/csp_iflist.h>
#include <csp/csp_sfp.h>
#include <csp/csp_promisc.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Max timeout */
#define CSP_MAX_TIMEOUT (UINT32_MAX)

/** Max delay */
#define CSP_MAX_DELAY CSP_MAX_TIMEOUT
#define CSP_INFINITY CSP_MAX_TIMEOUT


/**
 * CSP Debug Types
 */
enum csp_dedup_types {
   CSP_DEDUP_OFF,              /**< Deduplication off */
   CSP_DEDUP_FWD,              /**< Deduplication on forwarding only */
   CSP_DEDUP_INCOMING,         /**< Deduplication on incomfing only */
   CSP_DEDUP_ALL,              /**< Deduplication on incoming and forwarding*/
};

/**
 * CSP configuration.
 */
typedef struct csp_conf_s {
   uint8_t version;            /**< Protocol version to use (either 1 or 2) */
   const char *hostname;       /**< Host name, returned by the #CSP_CMP_IDENT request */
   const char *model;          /**< Model, returned by the #CSP_CMP_IDENT request */
   const char *revision;       /**< Revision, returned by the #CSP_CMP_IDENT request */
   uint32_t conn_dfl_so;       /**< Default connection options. Options will always be or'ed onto new connections, see csp_connect() */
   uint8_t dedup;              /**< Enable CSP deduplication. 0 = off, 1 = always on, 2 = only on forwarded packets,  */
} csp_conf_t;

extern csp_conf_t csp_conf;

/**
 * Initialize CSP.
 * This will configure basic structures.
 */
void csp_init(void);

/**
 * Free allocated resorces in CSP.
 * This is intended for testing of CSP, in order to be able re-initialize CSP by calling csp_init() again.
 */
void csp_free_resources(void);

/**
 * Get a \a read-only reference to the active CSP configuration.
 * Returns:
 *  Active CSP configuration (read-only).
 */
const csp_conf_t * csp_get_conf(void);

/**
 * Copy csp id fields from source to target object
 */
void csp_id_copy(csp_id_t * target, csp_id_t * source);

/**
 * Wait/accept a new connection.
 *
 * Parameters:
 *  socket (csp_scoket_t *) [in]: socket to accept connections on, created by calling csp_socket().
 *  timeout (uint32_t) [in]: timeout in mS to wait for a connection, use CSP_MAX_TIMEOUT for infinite timeout.
 *
 * Returns:
 *  csp_conn_t *: New connection on success, NULL on failure or timeout.
 */
csp_conn_t *csp_accept(csp_socket_t *socket, uint32_t timeout);

/**
 * Read packet from a connection.
 * This fuction will wait on the connection's RX queue for the specified timeout.
 *
 * Parameters:
 *  conn (csp_conn_t *) [in]: connection
 *  timeout (uint32_t) [in]: timeout in mS to wait for a packet, use CSP_MAX_TIMEOUT for infinite timeout.
 *
 * Returns:
 *  csp_packet_t *: Packet or NULL in case of failure or timeout.
 */
csp_packet_t *csp_read(csp_conn_t *conn, uint32_t timeout);

/**
 * Send packet on a connection.
 * The packet buffer is automatically freed, and cannot be used after the call to csp_send()
 *
 * Parameters:
 *  conn (csp_conn_t *) [in]: connection
 *  packet (csp_packet_t *) [in]: packet to send
*/
void csp_send(csp_conn_t *conn, csp_packet_t *packet);

/**
 * Change the default priority of the connection and send a packet.
 *
 * .. note:: The priority of the connection will be changed.
 *           If you need to change it back, call csp_send_prio() again.
 *
 * Parameters:
 *  prio (uint8_t) [in]: priority to set on the connection
 *  conn (csp_conn_t *) [in]: connection
 *  packet (csp_packet_t *) [in]: packet to send
 */
void csp_send_prio(uint8_t prio, csp_conn_t *conn, csp_packet_t *packet);

/**
 * Perform an entire request & reply transaction.
 * Creates a connection, send \a outbuf, wait for reply, copy reply to \a inbuf and close the connection.
*
* Parameters:
*   prio (uint8_t) [in]: priority, see #csp_prio_t
*   dst (uint16_t) [in]: destination address
*   dst_port (uint8_t) [in]: destination port
*   timeout (uint32_t) [in]: timeout in mS to wait for a reply
*   outbuf (void *) [in]: outgoing data (request)
*   outlen (int) [in]: length of data in \a outbuf (request)
*   inbuf (void *) [out]: user provided buffer for receiving data (reply)
*   inlen (int) [in]: length of expected reply, -1 for unknown size (inbuf MUST be large enough), 0 for no reply.
*   opts (uint32_t) [in]: connection options, see @ref CSP_CONNECTION_OPTIONS.
*
* Returns:
*   int: 1 or reply size on success, 0 on failure (error, incoming length does not match, timeout)
*/
int csp_transaction_w_opts(uint8_t prio, uint16_t dst, uint8_t dst_port, uint32_t timeout, void *outbuf, int outlen, void *inbuf, int inlen, uint32_t opts);

/**
 * Perform an entire request & reply transaction.
 * Creates a connection, send \a outbuf, wait for reply, copy reply to \a inbuf and close the connection.
 *
 * Parameters:
 *  prio (uint8_t) [in]: priority, see #csp_prio_t
 *  dest (uint16_t) [in]: destination address
 *  port (uint8_t) [in]: destination port
 *  timeout (uint32_t) [in]: timeout in mS to wait for a reply
 *  outbuf (void *) [in]: outgoing data (request)
 *  outlen (int) [in]: length of data in \a outbuf (request)
 *  inbuf (void *) [out]: user provided buffer for receiving data (reply)
 *  inlen (int) [in]: length of expected reply, -1 for unknown size (inbuf MUST be large enough), 0 for no reply.
 *
 * Returns:
 *  int: 1 or reply size on success, 0 on failure (error, incoming length does not match, timeout)
 */
static inline int csp_transaction(uint8_t prio, uint16_t dest, uint8_t port, uint32_t timeout, void * outbuf, int outlen, void * inbuf, int inlen) {
   return csp_transaction_w_opts(prio, dest, port, timeout, outbuf, outlen, inbuf, inlen, 0);
}

/**
 * Perform an entire request & reply transaction on an existing connection.
 * Send \a outbuf, wait for reply and copy reply to \a inbuf.
 *
 * Parameters:
 *  conn (csp_conn_t *) [in]: connection
 *  timeout (uint32_t) [in]: timeout in mS to wait for a reply
 *  outbuf (void *) [in]: outgoing data (request)
 *  outlen (int) [in]: length of data in \a outbuf (request)
 *  inbuf (void *) [out]: user provided buffer for receiving data (reply)
 *  inlen (int) [in]: length of expected reply, -1 for unknown size (inbuf MUST be large enough), 0 for no reply.
 *
 * Returns:
 *  int: 1 or reply size on success, 0 on failure (error, incoming length does not match, timeout)
 */
int csp_transaction_persistent(csp_conn_t *conn, uint32_t timeout, void *outbuf, int outlen, void *inbuf, int inlen);

/**
 * Read data from a connection-less server socket.
 *
 * Parameters:
 *  socket [in]: connection-less socket.
 *  timeout [in]: timeout in mS to wait for a packet, use #CSP_MAX_TIMEOUT for infinite timeout.
 *
 * Returns:
 *  csp_packet_t *: Packet on success, or NULL on failure or timeout.
 */
csp_packet_t *csp_recvfrom(csp_socket_t *socket, uint32_t timeout);

/**
 * Send a packet (without connection).
 *
 * Parameters:
 *  prio (uint8_t) [in]: packet priority, see #csp_prio_t
 *  dst (uint16_t) [in]: destination address
 *  dst_port (uint8_t) [in]: destination port
 *  src_port (uint8_t) [in]: source port
 *  opts (uint32_t) [in]: connection options, see @ref CSP_CONNECTION_OPTIONS.
 *  packet (csp_packet_t *) [in]: packet to send
 */
void csp_sendto(uint8_t prio, uint16_t dst, uint8_t dst_port, uint8_t src_port, uint32_t opts, csp_packet_t *packet);

/**
 * Send a packet as a reply to a request (without a connection).
 * Calls csp_sendto() with the source address and port from the request.
 *
 * Parameters:
 *  request (const csp_packet_t *) [in]: incoming request
 *  reply (csp_packet_t *) [in]: reply packet
 *  opts (uint32_t) [in]: connection options, see @ref CSP_CONNECTION_OPTIONS.
 */
void csp_sendto_reply(const csp_packet_t * request, csp_packet_t * reply, uint32_t opts);

/**
 * Establish outgoing connection.
 * The call will return immediately, unless it is a RDP connection (#CSP_O_RDP) in which case it will wait until the other
 * end acknowleges the connection (timeout is determined by the current connection timeout set by csp_rdp_set_opt()).
 *
 * Parameters:
 *  prio (uint8_t) [in]: priority, see #csp_prio_t
 *  dst (uint16_t) [in]: Destination address
 *  dst_port (uint8_t) [in]: Destination port
 *  timeout (uint32_t) [in]: unused.
 *  opts (uint32_t) [in]: connection options, see @ref CSP_CONNECTION_OPTIONS.
 *
 * Returns:
 *  csp_conn_t *: Established connection or NULL on failure (no free connections, timeout).
*/
csp_conn_t *csp_connect(uint8_t prio, uint16_t dst, uint8_t dst_port, uint32_t timeout, uint32_t opts);

/**
 * Close an open connection.
 * Any packets in the RX queue will be freed.
 *
 * Parameters:
 *  conn (sp_conn_t *) [in]: connection. Closing a NULL connection is acceptable.
 *
 * Returns:
 *  int: #CSP_ERR_NONE on success, otherwise an error code.
 */
int csp_close(csp_conn_t *conn);

/**
 * Close a socket, freeing it's RX queue and unbinding it from the associated
 * port.
 *
 * Parameters:
 *  sock (csp_scoket_t *) [in]: Socket
 *
 * Returns:
 *  int: #CSP_ERR_NONE on success, otherwise an error code.
 */
int csp_socket_close(csp_socket_t* sock);

/**
 * Return destination port of connection.
 *
 * Parameters:
 *  conn (csp_conn_t *) [in]: connection
 *
 * Returns:
 *  int: destination port of an incoming connection
 */
int csp_conn_dport(csp_conn_t *conn);

/**
 * Return source port of connection.
 *
 * Parameters:
 *  conn (csp_conn_t *) [in]: connection
 *
 * Returns:
 *  int: source port of an incoming connection
 */
int csp_conn_sport(csp_conn_t *conn);

/**
 * Return destination address of connection.
 *
 * Parameters:
 *  conn (csp_conn_t *) [in]: connection
 *
 * Returns:
 *  int: destination address of an incoming connection
 */
int csp_conn_dst(csp_conn_t *conn);

/**
 * Return source address of connection.
 *
 * Parameters:
 *  conn (csp_conn_t *) [in]: connection
 *
 * Returns:
 *  int: source address of an incoming connection
 */
int csp_conn_src(csp_conn_t *conn);

/**
 * Return flags of connection.
 *
 * Parameters:
 *  conn (csp_conn_t *) [in]: connection
 *
 * Returns:
 *  int:  flags of an incoming connection, see @ref CSP_HEADER_FLAGS
 */
int csp_conn_flags(csp_conn_t *conn);

/**
 * Set socket to listen for incoming connections.
 *
 * Parameters:
 *  socket (csp_socket_t *) [in]: socket
 *  backlog (size_t) [in]: max length of backlog queue. The backlog queue holds incoming connections, waiting to be returned by call to csp_accept().
 *
 * Returns:
 *  int: #CSP_ERR_NONE on success, otherwise an error code.
*/
int csp_listen(csp_socket_t *socket, size_t backlog);

/**
 * Bind port to socket.
 *
 * Parameters:
 *  socket (csp_socket_t *) [in]: socket to bind port to
 *  port (uint8_t) [in]: port number to bind, use #CSP_ANY for all ports. Bindnig to a specific will take precedence over #CSP_ANY.
 *
 * Returns:
 *  int: #CSP_ERR_NONE on success, otherwise an error code.
 */
int csp_bind(csp_socket_t *socket, uint8_t port);


/**
 * Bind port to callback function.
 *
 * Parameters:
 *  callback (csp_callback_t) [in]: pointer to callback function
 *  port (uint8_t) [in]: port number to bind, use #CSP_ANY for all ports. Bindnig to a specific will take precedence over #CSP_ANY.
 *
 * Returns:
 *  int: 0 on success, otherwise an error code.
 */
int csp_bind_callback(csp_callback_t callback, uint8_t port);

/**
 * Route packet from the incoming router queue and check RDP timeouts.
 * In order for incoming packets to routed and RDP timeouts to be checked, this function must be called reguarly.
 *
 * Returns:
 *  int: #CSP_ERR_NONE on success, otherwise an error code.
 */
int csp_route_work(void);

/**
 * Set the bridge interfaces.
 *
 * Parameters:
 *  if_a (csp_iface_t *) [in]: CSP Interface `A`
 *  if_b (csp_iface_t *) [in]: CSP Interface `B`
 */
void csp_bridge_set_interfaces(csp_iface_t * if_a, csp_iface_t * if_b);

/**
 * Bridge packet from an interface to the other.
 */
void csp_bridge_work(void);

/**
 * Handle CSP service request.
 * If the given packet is a service-request (the destination port matches one of CSP service ports #csp_service_port_t),
 * the packet will be processed by the specific CSP service handler.
 * The packet will either process it or free it, so this function is typically called in the last "default" clause of
 * a switch/case statement in a CSP listener task.
 * In order to listen to csp service ports, bind your listener to the specific services ports #csp_service_port_t or
 * use #CSP_ANY to all ports.
 *
 * Parameters:
 *  packet (csp_packet_t *) [in]: first packet, obtained by using csp_read()
 */
void csp_service_handler(csp_packet_t *packet);

/**
 * Send a single ping/echo packet.
 *
 * Parameters:
 *  node (uint16_t) [in]: address of subsystem.
 *  timeout (uint32_t) [in]: timeout in ms to wait for reply.
 *  size (unsigned int) [in]: payload size in bytes.
 *  opts (uint8_t) [in]: connection options, see @ref CSP_CONNECTION_OPTIONS.
 *
 * Returns:
 *  int: >0 = echo time in mS on success, otherwise -1 for error.
 */
int csp_ping(uint16_t node, uint32_t timeout, unsigned int size, uint8_t opts);

/**
 * Send a single ping/echo packet without waiting for reply.
 * Payload is 1 byte.
 *
 * Parameters:
 *  node (uint16_t) [in]: address of subsystem.
 */
void csp_ping_noreply(uint16_t node);

/**
 * Request process list.
 *
 * .. note:: This is currently only supported on FreeRTOS systems.
 *
 * Parameters:
 *  node (uint16_t) [in]: address of subsystem.
 *  timeout (uint32_t) [in]: timeout in mS to wait for replies. The function will not return until the timeout occurrs.
 */
void csp_ps(uint16_t node, uint32_t timeout);

/**
 * Request free memory.
 *
 * Parameters:
 *  node (uint16_t) [in]: address of subsystem.
 *  timeout (uint16_t) [in]: timeout in mS to wait for reply.
 *  size (uint32_t) [out]: free memory on subsystem.
 *
 * Returns:
 *  int: #CSP_ERR_NONE on success, otherwise an error code.
 */
int csp_get_memfree(uint16_t node, uint32_t timeout, uint32_t * size);

/**
 * Request free memory and print to stdout.
 *
 * Parameters:
 *  node (uint16_t) [in]: address of subsystem.
 *  timeout (uint32_t) [in]: timeout in mS to wait for reply.
 */
void csp_memfree(uint16_t node, uint32_t timeout);

/**
 * Request free buffers.
 *
 * Parameters:
 *  node (uint16_t) [in]: address of subsystem.
 *  timeout (uint32_t) [in]: timeout in mS to wait for reply.
 *  size (uint32_t) [out]: free buffers.
 *
 * Returns:
 *  int: #CSP_ERR_NONE on success, otherwise an error code.
*/
int csp_get_buf_free(uint16_t node, uint32_t timeout, uint32_t * size);

/**
 * Request free buffers and print to stdout.
 *
 * Parameters:
 *  node (uint16_t) [in]: address of subsystem.
 *  timeout (uint32_t) [in]: timeout in mS to wait for reply.
 *
 */
void csp_buf_free(uint16_t node, uint32_t timeout);

/**
 * Reboot subsystem.
 * If handled by the standard CSP service handler, the reboot handler set by csp_sys_set_reboot() on the subsystem, will be invoked.
 *
 * Parameters:
 *  node (uint16_t) [in]: address of subsystem.
 *
 */
void csp_reboot(uint16_t node);

/**
 * Shutdown subsystem.
 * If handled by the standard CSP service handler, the shutdown handler set by csp_sys_set_shutdown() on the subsystem, will be invoked.
 *
 * Parameters:
 *  node (uint16_t) [in]: address of subsystem.
 *
 */
void csp_shutdown(uint16_t node);

/**
 * Request uptime and print to stdout.
 *
 * Parameters:
 *  node (uint16_t) [in]: address of subsystem.
 *  timeout (uint32_t) [in]: timeout in mS to wait for reply.
 *
 */
void csp_uptime(uint16_t node, uint32_t timeout);

/**
 * Request uptime
 *
 * Parameters:
 *  node (uint16_t) [in]: address of subsystem.
 *  timeout (uint32_t) [in]: timeout in mS to wait for reply.
 *  uptime (uint32_t *) [out]: uptime in seconds.
 *
 * Returns:
 *  int: #CSP_ERR_NONE on success, otherwise an error code.
 */
int csp_get_uptime(uint16_t node, uint32_t timeout, uint32_t * uptime);

/**
 * Set RDP options.
 * The RDP options are used from the connecting/client side. When a RDP connection is established, the client tranmits the options to the server.
 *
 * Parameters:
 *  window_size (unsigned int) [in]: window size
 *  conn_timeout_ms (unsigned int) [in]: connection timeout in mS
 *  packet_timeout_ms (unsigned int) [in]: packet timeout in mS.
 *  delayed_acks (unsigned int) [in]: enable/disable delayed acknowledgements.
 *  ack_timeout (unsigned int) [in]: acknowledgement timeout when delayed ACKs is enabled
 *  ack_delay_count (unsigned int) [in]: send acknowledgement for every ack_delay_count packets.
 *
 */
void csp_rdp_set_opt(unsigned int window_size, unsigned int conn_timeout_ms,
	  unsigned int packet_timeout_ms, unsigned int delayed_acks,
	  unsigned int ack_timeout, unsigned int ack_delay_count);

/**
 * Get RDP options. @see csp_rdp_set_opt()
 *
 * Parameters:
 *  window_size (unsigned int *): Window size
 *  conn_timeout_ms (unsigned int *): connection timeout in ms
 *  packet_timeout_ms (unsigned int *): packet timeout in ms
 *  delayed_acks (unsigned int *): enable/disable delayed acknowledgements
 *  ack_timeout (unsigned int *): acknowledgement timeout when delayed ACKs is enabled
 *  ack_delay_count (unsigned int *): send acknowledgement for every ack_delay_count packets
 *
 * Returns:
 *  void:
 */
void csp_rdp_get_opt(unsigned int *window_size, unsigned int *conn_timeout_ms,
	  unsigned int *packet_timeout_ms, unsigned int *delayed_acks,
	  unsigned int *ack_timeout, unsigned int *ack_delay_count);

/**
 * Set platform specific memory copy function.
 */
void csp_cmp_set_memcpy(csp_memcpy_fnc_t fnc);

#if (CSP_ENABLE_CSP_PRINT)

/**
 * Print connection table to stdout.
 */
void csp_conn_print_table(void);

/**
 * Hex dump memory to stdout.
 *
 * Parameters:
 *  desc (const char *) [in] description printed on first line.
 *  addr (void *) [in] memory address.
 *  len (int) [in] number of bytes to dump, starting from \a addr.
 *
 */
void csp_hex_dump(const char *desc, void *addr, int len);

#else

inline void csp_conn_print_table(void) {}
inline void csp_hex_dump(const char *desc, void *addr, int len) {}

#endif

#if (CSP_HAVE_STDIO)
/**
 * Print connection table to string.
 */
int csp_conn_print_table_str(char * str_buf, int str_size);
#else
inline int csp_conn_print_table_str(char * str_buf, int str_size) {
   if (str_buf != NULL && str_size > 0) {
	  str_buf[0] = '\0';
   }

   return CSP_ERR_NONE;
}
#endif

#ifdef __cplusplus
}
#endif
