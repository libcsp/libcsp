#include <csp/csp_sfp.h>

#include <stdlib.h>

#include <csp/csp_buffer.h>
#include <csp/csp_crc32.h>
#include <csp/crypto/csp_hmac.h>
#include <csp/csp_debug.h>
#include "csp_macro.h"
#include <endian.h>

#include "csp_conn.h"

typedef struct __packed {
	uint32_t offset;
	uint32_t totalsize;
} sfp_header_t;

/**
 * SFP Headers:
 * The following functions are helper functions that handles the extra SFP
 * information that needs to be appended to all data packets.
 */
static inline sfp_header_t * csp_sfp_header_add(csp_packet_t * packet) {

	sfp_header_t * header = (sfp_header_t *)&packet->data[packet->length];
	packet->length += sizeof(*header);
	return header;
}

static inline sfp_header_t * csp_sfp_header_remove(csp_packet_t * packet) {

	if ((packet->id.flags & CSP_FFRAG) == 0) {
		return NULL;
	}
	sfp_header_t * header;
	if (packet->length < sizeof(*header)) {
		return NULL;
	}
	header = (sfp_header_t *)&packet->data[packet->length - sizeof(*header)];
	packet->length -= sizeof(*header);

	header->offset = be32toh(header->offset);
	header->totalsize = be32toh(header->totalsize);

	if (header->offset > header->totalsize) {
		return NULL;
	}

	return header;
}

uint32_t csp_sfp_opts_max_mtu(uint32_t opts) {
    uint32_t overhead = 0;
    
    /* If RDP is set, we must take RDP header into account. */
    if (opts & CSP_O_RDP) {
        overhead += CSP_RDP_HEADER_SIZE;
    }

    /* If CRC is set, we must take CRC size into account. */
    if (opts & CSP_O_CRC32) {
        overhead += sizeof(csp_crc32_t);
    }

    /* If HMAC is set, we must take HMAC header into account. */
    if (opts & CSP_O_HMAC) {
        overhead += CSP_HMAC_LENGTH;
    }

    /* Add SFP header size always */
    overhead += sizeof(sfp_header_t);
    
    return CSP_BUFFER_SIZE - overhead;
}

uint32_t csp_sfp_conn_max_mtu(const csp_conn_t * conn) {
    uint32_t max_mtu = 0;

    if (NULL != conn) {
        max_mtu = csp_sfp_opts_max_mtu(conn->opts);
    }

    return max_mtu;
}

int csp_sfp_send(csp_conn_t * conn, const csp_sfp_read_t * user, uint32_t totalsize, uint32_t mtu, uint32_t timeout) {
	(void)timeout;
	
	if ((NULL == conn) || (NULL == user) || (NULL == user->read)) {
		return CSP_ERR_INVAL;
	} else {
		uint32_t max_mtu = csp_sfp_conn_max_mtu(conn);
        
		if ((mtu > max_mtu) || (0 == mtu)) {
			return CSP_ERR_MTU;
		}
	}

	int error = CSP_ERR_NONE;
	uint32_t count = 0;
	while ((count < totalsize) && csp_conn_is_active(conn)) {

		sfp_header_t * sfp_header;

		/* Allocate packet */
		csp_packet_t * packet = csp_buffer_get(0);
		if (packet == NULL) {
			return CSP_ERR_NOMEM;
		}

		/* Calculate sending size */
		uint32_t size = totalsize - count;
		if (size > mtu) {
			size = mtu;
		}

		/* Print debug */
		//csp_print("%s: %d:%d, sending at %p size %u\n", __func__, csp_conn_src(conn), csp_conn_sport(conn), (void *)((uint8_t *)data + count), size);

		/* Copy data */
		error = user->read(packet->data, size, count, user->data);
		if (CSP_ERR_NONE != error) {
			csp_buffer_free(packet);
			return error;
		}

		packet->length = size;

		/* Set fragment flag */
		conn->idout.flags |= CSP_FFRAG;

		/* Add SFP header */
		sfp_header = csp_sfp_header_add(packet);
		sfp_header->totalsize = htobe32(totalsize);
		sfp_header->offset = htobe32(count);

		/* Send data */
		csp_send(conn, packet);

		/* Increment count */
		count += size;
	}

	return CSP_ERR_NONE;
}

int csp_sfp_recv_fp(csp_conn_t * conn, const csp_sfp_recv_t * user, uint32_t timeout, csp_packet_t * first_packet) {

    if ((NULL == conn) || (NULL == user) || (NULL == user->write)) {
        return CSP_ERR_INVAL;
    }

	/* Get first packet from user, or from connection */
	csp_packet_t * packet;
	if (first_packet == NULL) {
		packet = csp_read(conn, timeout);
		if (packet == NULL) {
			return CSP_ERR_TIMEDOUT;
		}
	} else {
		packet = first_packet;
	}

    const uint32_t max_mtu = csp_sfp_conn_max_mtu(conn);
	uint32_t datasize = 0;
	uint32_t data_offset = 0;
	int error = CSP_ERR_TIMEDOUT;
	do {
		/* Read SFP header */
		sfp_header_t * sfp_header = csp_sfp_header_remove(packet);
		if (sfp_header == NULL) {
			//csp_print("%s: %u:%u, invalid message, id.flags: 0x%x, length: %u\n", __func__, packet->id.src, packet->id.sport, packet->id.flags, packet->length);
			csp_buffer_free(packet);

			error = CSP_ERR_SFP;
			goto error;
		}

		//csp_print("%s: %u:%u, fragment %" PRIu32 "/%" PRIu32 "\n",  __func__, packet->id.src, packet->id.sport, sfp_header->offset + packet->length, sfp_header->totalsize);

		/* Consistency check */
		if (sfp_header->offset != data_offset) {
			//csp_print("%s: %u:%u, invalid message, offset %" PRIu32 " (expected %" PRIu32 "), length: %u, totalsize %" PRIu32 "\n", __func__, packet->id.src, packet->id.sport, sfp_header->offset, data_offset, packet->length, sfp_header->totalsize);
			csp_buffer_free(packet);

			error = CSP_ERR_SFP;
			goto error;
		}

		/* Ensure the packet length does not exceed maximum MTU. We can't receive length > MAX MTU. 
         * Ensure packet length is > 0. We can't receive length <= 0 */
        if ((max_mtu < packet->length) || (0 >= packet->length)) {
            csp_buffer_free(packet);

			error = CSP_ERR_SFP;
			goto error;
        }

		/* Check if the total size in the SFP header is zero, which is invalid for data transfer. */
        if (0 == sfp_header->totalsize) {
            csp_buffer_free(packet);

			error = CSP_ERR_SFP;
			goto error;
        }

        /* Set total expected size. This is done only on the 1st iteration. */
		if (datasize == 0) {
			datasize = sfp_header->totalsize;
		}

        /* Mismatch in total size */
        if (datasize != sfp_header->totalsize) {
            csp_buffer_free(packet);
            
			error = CSP_ERR_SFP;
			goto error;
        }

        /* Ensure the offset does not exceed the expected total size. */
        if (sfp_header->offset > (datasize - packet->length)) {
			csp_buffer_free(packet);

			error = CSP_ERR_SFP;
			goto error;
        }

		/* Consistency check */
		if (((data_offset + packet->length) > datasize) || (datasize != sfp_header->totalsize)) {
			//csp_print("%s: %u:%u, invalid size, sfp.offset: %" PRIu32 ", length: %u, total: %" PRIu32 " / %" PRIu32 "\n", __func__, packet->id.src, packet->id.sport, sfp_header->offset, packet->length, datasize, sfp_header->totalsize);
			csp_buffer_free(packet);

			error = CSP_ERR_SFP;
			goto error;
		}

		/* Copy data to output */
		error = user->write(packet->data, packet->length, data_offset, datasize, user->data);
		if (CSP_ERR_NONE != error) {
            csp_buffer_free(packet);
            goto error;
		}

		data_offset += packet->length;

		if (data_offset >= datasize) {
			// transfer complete
			csp_buffer_free(packet);

			return CSP_ERR_NONE;
		}

		/* Consistency check */
		if (packet->length == 0) {
			//csp_print("%s: %u:%u, invalid size, sfp.offset: %" PRIu32 ", length: %u, total: %" PRIu32 " / %" PRIu32 "\n", __func__, packet->id.src, packet->id.sport, sfp_header->offset, packet->length, datasize, sfp_header->totalsize);
			csp_buffer_free(packet);

			error = CSP_ERR_SFP;
			goto error;
		}

		csp_buffer_free(packet);

	} while ((packet = csp_read(conn, timeout)) != NULL);

error:
	return error;
}
