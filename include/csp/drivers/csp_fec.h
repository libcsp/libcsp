#ifndef CSP_FEC_DEFH
#define CSP_FEC_DEFH

#include <fec.h>
#include <csp/csp_platform.h>

#ifdef __cplusplus
extern "C" {
#endif

bool fec_csp_to_mpdu(mac_t *my_mac, csp_packet_t *packet, int mtu);

bool fec_mpdu_to_csp(mac_t *my_mac, const uint8_t *mpdu, csp_packet_t **packet, int mtu);

#ifdef __cplusplus
}
#endif
#endif // CSP_FEC_DEFH

