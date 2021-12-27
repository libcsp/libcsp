

#include "csp_dedup.h"

#include <stdlib.h>

#include <csp/arch/csp_time.h>
#include <csp/csp_crc32.h>
#include <csp/csp_id.h>

/* Check the last CSP_DEDUP_COUNT packets for duplicates */
#define CSP_DEDUP_COUNT 16

/* Only consider packet a duplicate if received under CSP_DEDUP_WINDOW_MS ago */
#define CSP_DEDUP_WINDOW_MS 100

/* Store packet CRC's in a ringbuffer */
static uint32_t csp_dedup_array[CSP_DEDUP_COUNT] = {};
static uint32_t csp_dedup_timestamp[CSP_DEDUP_COUNT] = {};
static int csp_dedup_in = 0;

bool csp_dedup_is_duplicate(csp_packet_t * packet) {
	/* Calculate CRC32 for packet */
	csp_id_prepend(packet);
	uint32_t crc = csp_crc32_memory(packet->frame_begin, packet->frame_length);

	/* Check if we have received this packet before */
	for (int i = 0; i < CSP_DEDUP_COUNT; i++) {

		/* Check for match */
		if (crc == csp_dedup_array[i]) {

			/* Check the timestamp */
			if (csp_get_ms() < csp_dedup_timestamp[i] + CSP_DEDUP_WINDOW_MS) {
				return true;
			}
		}
	}

	/* If not, insert packet into duplicate list */
	csp_dedup_array[csp_dedup_in] = crc;
	csp_dedup_timestamp[csp_dedup_in] = csp_get_ms();
	csp_dedup_in = (csp_dedup_in + 1) % CSP_DEDUP_COUNT;

	return false;
}
