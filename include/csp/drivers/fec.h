#ifndef FEC_DEFH
#define FEC_DEFH


#ifdef __cplusplus
extern "C" {
#endif
#include "MACWrapper.h"
#include <csp/csp_platform.h>
#include <error_correctionWrapper.h>
#include <rfModeWrapper.h>
#include <stdbool.h>

bool fec_csp_to_mpdu(mac_t *my_mac, csp_packet_t *packet, int mtu);

/* MPDU->CSP reconstructor state machine */
typedef enum {
    /* packet must be NULL initially */
    FEC_STATE_INIT = 0,
    /* MPDUs received but CSP packet not complete */
    FEC_STATE_IN_PROGRESS,
    /* missed data for current CSP packet - submit it, reset packet, call again */
    FEC_STATE_INCOMPLETE,
    /* all MPDUs for current CSP packet received - submit it and reset to NULL */
    FEC_STATE_COMPLETE,
    /* internal error, e.g. out of memory */
    FEC_STATE_ERROR,
} fec_state_t;

bool fec_mpdu_to_csp(mac_t *my_mac, const void *mpdu, csp_packet_t **packet, int mtu);

mac_t *fec_create(rf_mode_number_t rfmode, error_correction_scheme_t error_correction_scheme);

int fec_get_next_mpdu(mac_t *my_mac, void **);

int fec_get_mtu();

#ifdef __cplusplus
}
#endif
#endif // FEC_DEFH
