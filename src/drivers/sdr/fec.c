#include "csp/drivers/fec.h"
#include "csp/csp_endian.h"
#include <csp/csp_buffer.h>
#include "MACWrapper.h"
#include "rfModeWrapper.h"
#include <stdbool.h>
#include "radio.h"
#include <error_correctionWrapper.h>

#ifdef __cplusplus
extern "C" {
#endif

// each packet starts like this
struct mpdu {
    uint8_t csp_id; // what CSP packet we came from
    uint8_t index; // this packet index within the CSP packet
    uint16_t total_len; // total length of CSP packet
    uint8_t payload[0];
};

bool fec_csp_to_mpdu(mac_t *my_mac, csp_packet_t *packet, int mtu) {
    uint16_t len = packet->length;
	packet->id.ext = csp_hton32(packet->id.ext);
	packet->length = csp_hton16(len);

    return receive_packet(my_mac, (uint8_t *) packet, len + sizeof(csp_packet_t));
}

bool fec_mpdu_to_csp(mac_t *my_mac, const void *buf, csp_packet_t **packet, int mtu) {
    uhf_packet_processing_status_t rc = process_uhf_packet(my_mac, (const uint8_t *)buf, mtu);

    if (rc == PACKET_READY) {
        const csp_packet_t *rawCSP = (const csp_packet_t *) get_raw_packet_buffer(my_mac);

        uint16_t len = csp_ntoh16(rawCSP->length);
        csp_packet_t *clone = csp_buffer_get(len);
        if (clone) {
            memcpy(clone, rawCSP, csp_buffer_size());
            clone->length = len;
            clone->id.ext = csp_ntoh32(rawCSP->id.ext);
        }
        *packet = clone;
        return true;
    }
    return false;
}

mac_t *fec_create(rf_mode_number_t rfmode, error_correction_scheme_t error_correction_scheme) {
    return mac_create(rfmode, error_correction_scheme);
}

// Returns 0 if none exist, else returns mpdu size
int fec_get_next_mpdu(mac_t *my_mac, void **buf) {
    static uint32_t index = 0;
    const uint32_t length = mpdu_payloads_buffer_length(my_mac);
    if (index >= length) {
        index = 0;
        return 0;
    }
    const uint8_t *mpdusBuffer = mpdu_payloads_buffer(my_mac);
    const uint32_t mtu = raw_mpdu_length();
    *buf = (uint8_t *)(mpdusBuffer) + index;
    index += mtu;
    return mtu;
}

int fec_get_mtu() {
    return raw_mpdu_length();
}

#ifdef __cplusplus
}
#endif
