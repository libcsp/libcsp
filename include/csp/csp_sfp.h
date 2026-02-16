/*****************************************************************************
 * **File:** csp/csp_sfp.h
 *
 * **Description:** Simple Fragmentation Protocol (SFP).
 *
 * The SFP API can transfer a blob of data across an established CSP connection,
 * by chopping the data into smaller chunks of data, that can fit into a single CSP message.
 *
 * SFP will add a small header to each packet, containing information about the transfer.
 * SFP is usually sent over a RDP connection (which also adds a header),
 ****************************************************************************/
#pragma once

#include <csp/csp_types.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Structure to be passed as an input parameter to csp_sfp_send(...).
 * This structure encapsulates user-defined data and a function for reading data from storage.
 */
typedef struct {
    /**
     * User-defined data. SFP does not interpret or manipulate this data.
     * It can be any user-defined data such as a file handle, a raw buffer pointer, or any other 
     * relevant information required for the implementation. NULL is accepted.
     */
    void * data;

    /**
     * User-defined function for reading data from a storage medium.
     * 
     * This callback allows users to define a custom mechanism for retrieving data, such as reading 
     * from a file, memory buffer, or another type of storage. The SFP layer invokes this function 
     * during data transmission.
     * 
     * @param[out] buffer Pointer to the buffer where the read data should be stored. This pointer is always valid.
     * @param[in] size The number of bytes to read. Guaranteed to be greater than 0.
     * @param[in] offset The offset in the source storage from where the data should be read. This allows 
     *                   the user to retrieve data from specific positions, such as file offsets or 
     *                   buffer indices.
     * @param[in] data Pointer to user-specific data provided by the caller (e.g., a file handle, buffer state, 
     *                 or context). This pointer is optional and may be NULL, depending on the user implementation.
     * @return #CSP_ERR_NONE on success, otherwise an error.
     */
    int (* read)(uint8_t * buffer, uint32_t size, uint32_t offset, void * data);
} csp_sfp_read_t;

/**
 * Structure to be passed as an input parameter to csp_sfp_recv(...).
 * This structure encapsulates user-defined data and a function for writing data to storage.
 */
typedef struct {
    /**
     * User-defined data. SFP does not interpret or manipulate this data.
     * It can be any user-defined data such as a file handle, a raw buffer pointer, or any other 
     * relevant information required for the implementation. NULL is accepted.
     */
    void * data;

    /**
     * User-defined function for writing data to a storage medium.
     * 
     * This callback allows users to define a custom mechanism for handling incoming data, 
     * such as writing it to a file, buffer, or another type of storage. The SFP layer 
     * invokes this function during data reception.
     * 
     * @param[in] buffer Pointer to the buffer containing the data to be written. This pointer is always valid.
     * @param[in] size The number of bytes to write. Guaranteed to be greater than 0.
     * @param[in] offset The offset in the destination storage where the data should be written. This 
     *                   allows the user to place data at specific locations, such as file positions 
     *                   or buffer indices.
     * @param[in] totalsz The total expected size of the incoming data. This value is consistent 
     *                    across all calls to this function during a single transfer and can be used 
     *                    for validation or progress tracking.
     * @param[in] data Pointer to user-specific data provided by the caller (e.g., a file handle, 
     *                 buffer state, or context). This pointer is optional and may be NULL, depending 
     *                 on the user implementation.
     * @return #CSP_ERR_NONE on success, otherwise an error.
     */
    int (* write)(const uint8_t * buffer, uint32_t size, uint32_t offset, uint32_t totalsz, void * data);
} csp_sfp_recv_t;

/**
 * Get the maximum MTU (Maximum Transmission Unit) for options.
 *
 * @param opts Options indicating required protocol features (RDP, CRC, etc.).
 * @return The maximum MTU in bytes.
 */
uint32_t csp_sfp_opts_max_mtu(uint32_t opts);

/**
 * Get the maximum MTU (Maximum Transmission Unit) for a connection.
 *
 * @param conn Connection indicating required protocol features (RDP, CRC, etc.).
 * @return The maximum MTU in bytes.
 */
uint32_t csp_sfp_conn_max_mtu(const csp_conn_t * conn);

/**
 * Send data over a CSP connection.
 *
 * @param[in] conn established connection for sending SFP packets.
 * @param[in] user User-defined read function and data pointer.
 * @param[in] datasize Total size to send.
 * @param[in] mtu  maximum transfer unit (bytes), max data chunk to send.
 * @param[in] timeout unused as of CSP version 1.6
 * @return #CSP_ERR_NONE on success, otherwise an error.
 */
int csp_sfp_send(csp_conn_t * conn, const csp_sfp_read_t * user, uint32_t datasize, uint32_t mtu, uint32_t timeout);

/**
 * Receive data over a CSP connection.
 *
 * This is the counterpart to the csp_sfp_send() and csp_sfp_send_own_memcpy().
 *
 * @param[in] conn established connection for receiving SFP packets.
 * @param[in] user User-defined data with write function and data pointer.
 * @param[in] timeout timeout in ms to wait for csp_read()
 * @param[in] first_packet First packet of a SFP transfer.
 * 			  Use NULL to receive first packet on the connection.
 * @return #CSP_ERR_NONE on success, otherwise an error.
 */
int csp_sfp_recv_fp(csp_conn_t * conn, const csp_sfp_recv_t * user, uint32_t timeout, csp_packet_t * first_packet);

/**
 * Receive data over a CSP connection.
 *
 * This is the counterpart to the csp_sfp_send()
 *
 * @param[in] conn established connection for receiving SFP packets.
 * @param[in] user User-defined data with write function and data pointer.
 * @param[in] timeout timeout in ms to wait for csp_read()
 * @return #CSP_ERR_NONE on success, otherwise an error.
*/
static inline int csp_sfp_recv(csp_conn_t * conn, const csp_sfp_recv_t * user, uint32_t timeout) {
	return csp_sfp_recv_fp(conn, user, timeout, NULL);
}

#ifdef __cplusplus
}
#endif
