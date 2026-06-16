#include "csp_cmp_internal.h"

#include "csp_macro.h"

#include <endian.h>
#include <stdbool.h>
#include <stdint.h>

#include <csp/csp_hooks.h>

#define CSP_CMP_TIMESYNC_ENTRIES 2u
#define NS_PER_S 1000000000ull

typedef struct {
	uint16_t id;
	uint16_t master;
	uint32_t tv_sec;
	uint64_t rx_time;
	bool valid;
} csp_cmp_timesync_entry_t;

static csp_cmp_timesync_entry_t time_sync_entries[CSP_CMP_TIMESYNC_ENTRIES];
static uint8_t time_sync_next_idx;

__weak uint64_t csp_cmp_timesync_rx_time_hook(const csp_packet_t * packet) {
	return packet->timestamp_rx;
}

__weak void csp_cmp_timesync_fup_hook(const csp_cmp_timesync_fup_t * fup) {
	(void)fup;
}

static uint64_t csp_cmp_timesync_utc_ns(uint32_t tv_sec, int32_t correction_ns) {

	uint64_t origin_utc_ns = (uint64_t)tv_sec * NS_PER_S;

	if (correction_ns < 0) {
		uint64_t correction_abs_ns = (uint64_t)(-(int64_t)correction_ns);
		return (origin_utc_ns > correction_abs_ns) ? (origin_utc_ns - correction_abs_ns) : 0u;
	}

	return origin_utc_ns + (uint64_t)correction_ns;
}

int csp_cmp_time_sync_handler(csp_packet_t * packet) {

	if (packet->length != sizeof(struct csp_cmp_time_sync_msg)) {
		return CSP_ERR_INVAL;
	}

	const struct csp_cmp_time_sync_msg * msg = (const struct csp_cmp_time_sync_msg *)packet->data;
	csp_cmp_timesync_entry_t * entry = &time_sync_entries[time_sync_next_idx];

	entry->id = be16toh(msg->id);
	entry->master = packet->id.src;
	entry->tv_sec = be32toh(msg->tv_sec);
	entry->rx_time = csp_cmp_timesync_rx_time_hook(packet);
	entry->valid = true;

	time_sync_next_idx = (time_sync_next_idx + 1u) & (CSP_CMP_TIMESYNC_ENTRIES - 1u);

	return CSP_CMP_NO_REPLY;
}

int csp_cmp_time_fup_handler(csp_packet_t * packet) {

	if (packet->length != sizeof(struct csp_cmp_time_fup_msg)) {
		return CSP_ERR_INVAL;
	}

	const struct csp_cmp_time_fup_msg * msg = (const struct csp_cmp_time_fup_msg *)packet->data;
	uint16_t id = be16toh(msg->id);

	for (uint8_t idx = 0; idx < CSP_CMP_TIMESYNC_ENTRIES; idx++) {
		csp_cmp_timesync_entry_t * entry = &time_sync_entries[idx];

		if (!entry->valid || entry->id != id || entry->master != packet->id.src) {
			continue;
		}

		uint32_t tv_sec = be32toh(msg->tv_sec);
		if (tv_sec != entry->tv_sec) {
			entry->valid = false;
			break;
		}

		int32_t correction_ns = (int32_t)be32toh((uint32_t)msg->correction_ns);
		csp_cmp_timesync_fup_t fup = {
			.master = entry->master,
			.id = id,
			.tv_sec = tv_sec,
			.correction_ns = correction_ns,
			.sync_rx_time = entry->rx_time,
			.sync_utc_ns = csp_cmp_timesync_utc_ns(tv_sec, correction_ns),
		};

		csp_cmp_timesync_fup_hook(&fup);
		entry->valid = false;
		break;
	}

	return CSP_CMP_NO_REPLY;
}
