#include "fec.h"
#include <csp/csp_buffer.h>
#include <csp/drivers/sdr.h>
#include "MACWrapper.h"
#include "rfModeWrapper.h"
#include <stdbool.h>
#include "radio.h"
#include <error_correctionWrapper.h>

#ifdef __cplusplus
extern "C" {
#endif

static mac_t *my_mac;

// each packet starts like this
struct mpdu {
    uint8_t csp_id; // what CSP packet we came from
    uint8_t index; // this packet index within the CSP packet
    uint16_t total_len; // total length of CSP packet
    uint8_t payload[0];
};

bool fec_csp_to_mpdu(csp_packet_t *packet, int mtu) {
    return receive_csp_packet(my_mac, packet);
}

bool fec_mpdu_to_csp(const void *buf, csp_packet_t **packet, uint8_t *id, int mtu) {
    if (process_uhf_packet(my_mac, (const uint8_t *)buf, mtu) == CSP_PACKET_READY) {
        const uint8_t *rawCSP = get_raw_csp_packet_buffer(my_mac);
        *packet = csp_buffer_clone((void *)rawCSP);//(csp_packet_t *)newpacket;
        return true;
    }
    return false;
}

void fec_create(rf_mode_number_t rfmode, error_correction_scheme_t error_correction_scheme) {
    my_mac = mac_create(rfmode, error_correction_scheme);
}

// Returns 0 if none exist, else returns mpdu size
int fec_get_next_mpdu(void **buf) {
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
