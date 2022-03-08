#include "fec.h"
#include <csp/csp_buffer.h>
#include <csp/drivers/sdr.h>
#include "circular_buffer.h"
#include <util/service_utilities.h>
#include "MACWrapper.h"
#include "rfModeWrapper.h"
#include "os_semphr.h"
#include <stdbool.h>
#include "radio.h"
#include <error_correctionWrapper.h>


static mac_t mac;

// each packet starts like this
struct mpdu {
    uint8_t csp_id; // what CSP packet we came from
    uint8_t index; // this packet index within the CSP packet
    uint16_t total_len; // total length of CSP packet
    uint8_t payload[0];
};

// count of how many CSP packets we've sent
static uint32_t csp_index;

bool fec_csp_to_mpdu(CircularBufferHandle fifo, csp_packet_t *packet, int mtu) {
    return receive_csp_packet(mac, packet);
}

bool fec_mpdu_to_csp(const void *buf, csp_packet_t **packet, uint8_t *id, int mtu) {
    if (process_uhf_packet(mac, buf, mtu) == CSP_PACKET_READY) {
        const uint8_t *rawCSP = get_raw_csp_packet_buffer(mac);
        const uint32_t rawLen = get_raw_csp_packet_length(mac);
        uint8_t *newpacket = csp_malloc(rawLen);

        memcpy(newpacket, rawCSP, rawLen);

        *packet = (csp_packet_t *)newpacket;
        return true;
    }
    return false;
}

void fec_create(rf_mode_number_t rfmode, error_correction_scheme_t error_correction_scheme) {
    mac = mac_create(rfmode, error_correction_scheme);
}

// Returns 0 if none exist, else returns mpdu size
int fec_get_next_mpdu(void **buf) {
    static int index = 0;
    const uint32_t length = mpdu_payloads_buffer_length(mac);
    if (index >= length) {
        index = 0;
        return 0;
    }
    const uint8_t *mpdusBuffer = mpdu_payloads_buffer(mac);
    const uint32_t mtu = raw_mpdu_length();
    *buf = mpdusBuffer[index*mtu];
    index++;
    return mtu;
}

int fec_get_mtu() {
    return raw_mpdu_length();
}
