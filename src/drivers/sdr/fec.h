#ifndef FEC_DEFH
#define FEC_DEFH

#include <csp/csp_platform.h>
#include "circular_buffer.h"
#include <error_correctionWrapper.h>
#ifdef __cplusplus
extern "C" {
#endif

int fec_csp_to_mpdu(CircularBufferHandle fifo, csp_packet_t *packet, int mtu);

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

bool fec_mpdu_to_csp(const void *mpdu, csp_packet_t **packet, uint8_t *id, int mtu);

void fec_create(rf_mode_number_t rfmode, error_correction_scheme_t error_correction_scheme);

int fec_get_next_mpdu(void **);

int fec_get_mtu();

#ifdef __cplusplus
}
#endif
#endif // FEC_DEFH
