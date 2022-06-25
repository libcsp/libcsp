#include "csp/drivers/csp_fec.h"
#include "csp/csp_endian.h"
#include <csp/csp_buffer.h>
#include "MACWrapper.h"
#include "rfModeWrapper.h"
#include <stdbool.h>
#include "radio.h"
#include <error_correctionWrapper.h>
#include <string.h>

bool fec_csp_to_mpdu(mac_t *my_mac, csp_packet_t *packet, int mtu) {
    uint16_t len = packet->length;
	packet->id.ext = csp_hton32(packet->id.ext);
	packet->length = csp_hton16(len);

    return receive_packet(my_mac, (uint8_t *) packet, len + sizeof(csp_packet_t));
}

bool fec_mpdu_to_csp(mac_t *my_mac, const uint8_t *buf, csp_packet_t **packet, int mtu) {
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
