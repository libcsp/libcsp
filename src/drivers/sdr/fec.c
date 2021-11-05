#include <csp/csp_buffer.h>
#include <csp/drivers/sdr.h>
#include "fec.h"
#include "circular_buffer.h"
#include <util/service_utilities.h>

// each packet starts like this
struct mpdu {
    uint8_t csp_id; // what CSP packet we came from
    uint8_t index; // this packet index within the CSP packet
    uint16_t total_len; // total length of CSP packet
    uint8_t payload[0];
};

// count of how many CSP packets we've sent
static uint32_t csp_index;

int fec_csp_to_mpdu(CircularBufferHandle fifo, csp_packet_t *packet, int mtu) {
    uint8_t *cursor = (uint8_t *) packet;
    int remaining = sizeof(csp_packet_t) + packet->length;
    int mpdu_count = 0;
    csp_index++;
    while (remaining > 0) {
        int tries = 0;
        struct mpdu *head = (struct mpdu *) CircularBufferNextHead(fifo);
        while (!head && tries < 10) {
            vTaskDelay(100);
            head = (struct mpdu *) CircularBufferNextHead(fifo);
            ++tries;
            
        }
        if (!head) {
            ex2_log("fec_add: could not send packet");
            return mpdu_count;
        }
        head->csp_id = csp_index & 0x0ff;
        head->index = mpdu_count;
        head->total_len = sizeof(csp_packet_t) + packet->length;
        int payload_len = mtu - sizeof(struct mpdu);
        if (payload_len > remaining)
            payload_len = remaining;
        memcpy(head->payload, cursor, payload_len); 
        remaining -= payload_len;
        cursor += payload_len; // Advance the cursor through the stream
        ++mpdu_count;

        CircularBufferSend(fifo);
    }

    return mpdu_count;
}

fec_state_t fec_mpdu_to_csp(const void *buf, csp_packet_t **packet, uint8_t *id, int mtu) {
    fec_state_t state = FEC_STATE_IN_PROGRESS;
    const struct mpdu *mpdu = (const struct mpdu *) buf;
    if (!packet || !id) {
        ex2_log("missing cookies");
        return FEC_STATE_ERROR;
    }

    if (*packet == 0) {
        /* There is no current CSP packet and we got a new MPDU, so that means
         * we are starting a new CSP. Normally, this would also coincide with
         * the MPDU index being 0, but if it got dropped then c'est la vie.
         */
        if (mpdu->index != 0) {
            ex2_log("missing packets - first mpdu %d", mpdu->index);
        }
        /* The CSP infrastructure does not like getting malloc'ed packets,
         * so we use csp_get_buffer instead.
         */
        *packet = csp_buffer_get(mpdu->total_len - sizeof(csp_packet_t));
        if (*packet == 0) {
            ex2_log("out of memory");
            return FEC_STATE_ERROR;
        }
        (*packet)->length = mpdu->total_len - sizeof(csp_packet_t);
        *id = mpdu->csp_id; 
    }
    else if (mpdu->csp_id != *id) {
        /* We were working on a CSP packet with sequence# *id, but the new MPDU
         * has a different id. That must mean we dropped the last MPDUs of the
         * last CSP packet. Return INCOMPLETE so we can finish off the last 
         * packet and start this one.
         */
        ex2_log("missed packets? new id %d != %d", mpdu->csp_id, *id);
        return FEC_STATE_INCOMPLETE;
    }

    size_t start = mpdu->index*(mtu - sizeof(struct mpdu));
    if (start >= mpdu->total_len) {
        ex2_log("corrupt packet? index %d total_len %d", mpdu->index, mpdu->total_len);
        return state;
    }
    size_t len = mtu - sizeof(struct mpdu);
    size_t mpdu_offset = 0;
    /* last packet? */
    if (len >= ((size_t) mpdu->total_len - start)) {
        len = mpdu->total_len - start;
        state = FEC_STATE_COMPLETE;
    }

    if (mpdu->index == 0) {
        /* For the first packet skip over the CSP header in the MPDU payload,
         * since we had to use csp_get_buffer above to get a local CSP packet.
         */
        mpdu_offset = sizeof(csp_packet_t);
        len -= sizeof(csp_packet_t);
        (*packet)->id.ext = ((csp_packet_t *) mpdu->payload)->id.ext;
    }
    else {
        /* For all other MPDUs adjust the start to account for the CSP header
         */
        start -= sizeof(csp_packet_t);
    }
    ex2_log("mpdu %d, sizeof(csp) %d, start %d, offset %d, len %d", mpdu->index, sizeof(csp_packet_t), start, mpdu_offset, len);
    memcpy((*packet)->data + start, mpdu->payload + mpdu_offset, len);
    return state;
}
