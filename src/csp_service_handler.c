#include <csp/csp.h>

#include <csp/csp_debug.h>
#include <stddef.h>
#include <string.h>

#include <csp/csp_cmp.h>
#include <csp/csp_hooks.h>
#include <endian.h>
#include <csp/csp_types.h>
#include <csp/csp_rtable.h>
#include <csp/csp_id.h>
#include <csp/arch/csp_time.h>

/**
 * The CSP CMP mempy function is used to, override the function used to
 * read/write memory by peek and poke.
 */
#ifdef __AVR__
static uint32_t wrap_32bit_memcpy(uint32_t to, const uint32_t from, size_t size) {
	return (uint32_t)(uintptr_t)memcpy((void *)(uintptr_t)to, (const void *)(uintptr_t)from, size);
}
static csp_memcpy_fnc_t csp_cmp_memcpy_fnc = wrap_32bit_memcpy;
#else
static csp_memcpy_fnc_t csp_cmp_memcpy_fnc = (csp_memcpy_fnc_t)memcpy;
static csp_memread64_fnc_t csp_cmp_memread64_fnc = (csp_memread64_fnc_t)NULL;
static csp_memwrite64_fnc_t csp_cmp_memwrite64_fnc = (csp_memwrite64_fnc_t)NULL;
#endif

void csp_cmp_set_memcpy(csp_memcpy_fnc_t fnc) {
	csp_cmp_memcpy_fnc = fnc;
}

void csp_cmp_set_memread64(csp_memread64_fnc_t fnc) {
	csp_cmp_memread64_fnc = fnc;
}

void csp_cmp_set_memwrite64(csp_memwrite64_fnc_t fnc) {
	csp_cmp_memwrite64_fnc = fnc;
}

static int csp_cmp_check_len(const csp_packet_t * packet, size_t min_len) {

	if (packet->length < min_len) {
		return CSP_ERR_INVAL;
	}

	return CSP_ERR_NONE;
}

static int do_cmp_ident(csp_packet_t * packet) {

	struct csp_cmp_ident_msg * cmp = (struct csp_cmp_ident_msg *)packet->data;

	if (csp_cmp_check_len(packet, sizeof(*cmp)) != CSP_ERR_NONE) {
		return CSP_ERR_INVAL;
	}

	/* Copy revision */
	strncpy(cmp->revision, csp_conf.revision, CSP_CMP_IDENT_REV_LEN);
	cmp->revision[CSP_CMP_IDENT_REV_LEN - 1] = '\0';

#if CSP_REPRODUCIBLE_BUILDS == 0
	/* Copy compilation date */
	strncpy(cmp->date, __DATE__, CSP_CMP_IDENT_DATE_LEN);
	cmp->date[CSP_CMP_IDENT_DATE_LEN - 1] = '\0';

	/* Copy compilation time */
	strncpy(cmp->time, __TIME__, CSP_CMP_IDENT_TIME_LEN);
	cmp->time[CSP_CMP_IDENT_TIME_LEN - 1] = '\0';
#endif

	/* Copy hostname */
	strncpy(cmp->hostname, csp_conf.hostname, CSP_HOSTNAME_LEN);
	cmp->hostname[CSP_HOSTNAME_LEN - 1] = '\0';

	/* Copy model name */
	strncpy(cmp->model, csp_conf.model, CSP_MODEL_LEN);
	cmp->model[CSP_MODEL_LEN - 1] = '\0';

	packet->length = sizeof(*cmp);

	return CSP_ERR_NONE;
}

static int do_cmp_route_set_v1(csp_packet_t * packet) {

	struct csp_cmp_route_set_v1_msg * cmp = (struct csp_cmp_route_set_v1_msg *)packet->data;

	if (csp_cmp_check_len(packet, sizeof(*cmp)) != CSP_ERR_NONE) {
		return CSP_ERR_INVAL;
	}

	csp_iface_t * ifc = csp_iflist_get_by_name(cmp->interface);
	if (ifc == NULL) {
		return CSP_ERR_INVAL;
	}
#if CSP_USE_RTABLE
	if (csp_rtable_set(cmp->dest_node, csp_id_get_host_bits(), ifc, cmp->next_hop_via) != CSP_ERR_NONE) {
		return CSP_ERR_INVAL;
	}
#endif

	packet->length = sizeof(*cmp);

	return CSP_ERR_NONE;
}

static int do_cmp_route_set_v2(csp_packet_t * packet) {

	struct csp_cmp_route_set_v2_msg * cmp = (struct csp_cmp_route_set_v2_msg *)packet->data;

	if (csp_cmp_check_len(packet, sizeof(*cmp)) != CSP_ERR_NONE) {
		return CSP_ERR_INVAL;
	}

	csp_iface_t * ifc = csp_iflist_get_by_name(cmp->interface);
	if (ifc == NULL) {
		return CSP_ERR_INVAL;
	}

#if CSP_USE_RTABLE
	if (csp_rtable_set(be16toh(cmp->dest_node), be16toh(cmp->netmask), ifc, be16toh(cmp->next_hop_via)) != CSP_ERR_NONE) {
		return CSP_ERR_INVAL;
	}
#endif

	packet->length = sizeof(*cmp);

	return CSP_ERR_NONE;
}

static int do_cmp_if_stats(csp_packet_t * packet) {

	struct csp_cmp_if_stats_msg * cmp = (struct csp_cmp_if_stats_msg *)packet->data;

	if (csp_cmp_check_len(packet, offsetof(struct csp_cmp_if_stats_msg, tx)) != CSP_ERR_NONE) {
		return CSP_ERR_INVAL;
	}

	csp_iface_t * ifc = csp_iflist_get_by_name(cmp->interface);
	if (ifc == NULL)
		return CSP_ERR_INVAL;

	cmp->tx = htobe32(ifc->tx);
	cmp->rx = htobe32(ifc->rx);
	cmp->tx_error = htobe32(ifc->tx_error);
	cmp->rx_error = htobe32(ifc->rx_error);
	cmp->drop = htobe32(ifc->drop);
	cmp->autherr = htobe32(ifc->autherr);
	cmp->frame = htobe32(ifc->frame);
	cmp->txbytes = htobe32(ifc->txbytes);
	cmp->rxbytes = htobe32(ifc->rxbytes);
	cmp->irq = htobe32(ifc->irq);

	packet->length = sizeof(*cmp);

	return CSP_ERR_NONE;
}

static int do_cmp_peek(csp_packet_t * packet) {

	struct csp_cmp_peek_msg * cmp = (struct csp_cmp_peek_msg *)packet->data;

	if (csp_cmp_check_len(packet, sizeof(*cmp)) != CSP_ERR_NONE) {
		return CSP_ERR_INVAL;
	}

	cmp->addr = htobe32(cmp->addr);
	if (cmp->len > CSP_CMP_PEEK_MAX_LEN)
		return CSP_ERR_INVAL;

	/* Dangerous, you better know what you are doing */
	csp_cmp_memcpy_fnc((csp_memptr_t)(uintptr_t)cmp->data, (csp_memptr_t)(uintptr_t)cmp->addr, cmp->len);

	packet->length = CMP_PEEK_SIZE(cmp->len);

	return CSP_ERR_NONE;
}

static int do_cmp_poke(csp_packet_t * packet) {

	struct csp_cmp_poke_msg * cmp = (struct csp_cmp_poke_msg *)packet->data;

	if (csp_cmp_check_len(packet, sizeof(*cmp)) != CSP_ERR_NONE) {
		return CSP_ERR_INVAL;
	}

	cmp->addr = htobe32(cmp->addr);
	if (cmp->len > CSP_CMP_POKE_MAX_LEN)
		return CSP_ERR_INVAL;

	if (csp_cmp_check_len(packet, sizeof(*cmp) + cmp->len) != CSP_ERR_NONE) {
		return CSP_ERR_INVAL;
	}

	/* Extremely dangerous, you better know what you are doing */
	csp_cmp_memcpy_fnc((csp_memptr_t)(uintptr_t)cmp->addr, (csp_memptr_t)(uintptr_t)cmp->data, cmp->len);

	packet->length = CMP_POKE_SIZE(cmp->len);

	return CSP_ERR_NONE;
}

static int do_cmp_peek_v2(csp_packet_t * packet) {

	struct csp_cmp_peek_v2_msg * cmp = (struct csp_cmp_peek_v2_msg *)packet->data;

	if (csp_cmp_check_len(packet, sizeof(*cmp)) != CSP_ERR_NONE) {
		return CSP_ERR_INVAL;
	}

	cmp->vaddr = htobe64(cmp->vaddr);
	if (cmp->len > CSP_CMP_PEEK_V2_MAX_LEN)
		return CSP_ERR_INVAL;

	if (!csp_cmp_memread64_fnc) {
		return CSP_ERR_DRIVER;
	}

	/* Dangerous, you better know what you are doing */
	csp_cmp_memread64_fnc(cmp->data, cmp->vaddr, cmp->len);

	packet->length = CMP_PEEK_V2_SIZE(cmp->len);

	return CSP_ERR_NONE;
}

static int do_cmp_poke_v2(csp_packet_t * packet) {

	struct csp_cmp_poke_v2_msg * cmp = (struct csp_cmp_poke_v2_msg *)packet->data;

	if (csp_cmp_check_len(packet, sizeof(*cmp)) != CSP_ERR_NONE) {
		return CSP_ERR_INVAL;
	}

	cmp->vaddr = htobe64(cmp->vaddr);
	if (cmp->len > CSP_CMP_POKE_V2_MAX_LEN)
		return CSP_ERR_INVAL;

	if (!csp_cmp_memwrite64_fnc) {
		return CSP_ERR_DRIVER;
	}

	if (csp_cmp_check_len(packet, sizeof(*cmp) + cmp->len) != CSP_ERR_NONE) {
		return CSP_ERR_INVAL;
	}

	/* Extremely dangerous, you better know what you are doing */
	csp_cmp_memwrite64_fnc(cmp->vaddr, cmp->data, cmp->len);

	packet->length = CMP_POKE_V2_SIZE(cmp->len);

	return CSP_ERR_NONE;
}

static int do_cmp_clock(csp_packet_t * packet) {

	struct csp_cmp_clock_msg * cmp = (struct csp_cmp_clock_msg *)packet->data;

	if (csp_cmp_check_len(packet, sizeof(*cmp)) != CSP_ERR_NONE) {
		return CSP_ERR_INVAL;
	}

	csp_timestamp_t clock;
	clock.tv_sec = be32toh(cmp->clock.tv_sec);
	clock.tv_nsec = be32toh(cmp->clock.tv_nsec);

	int res = CSP_ERR_NONE;
	if (clock.tv_sec != 0) {
		// set time
		res = csp_clock_set_time(&clock);
		if (res != CSP_ERR_NONE) {
			csp_dbg_errno = CSP_DBG_ERR_CLOCK_SET_FAIL;
		}
	}

	csp_clock_get_time(&clock);

	cmp->clock.tv_sec = htobe32(clock.tv_sec);
	cmp->clock.tv_nsec = htobe32(clock.tv_nsec);

	packet->length = sizeof(*cmp);

	return res;
}

typedef int (*csp_cmp_service_fnc_t)(csp_packet_t * packet);

typedef struct {
	uint8_t code;
	csp_cmp_service_fnc_t handler;
} csp_cmp_service_t;

static const csp_cmp_service_t csp_cmp_services[] = {
	{CSP_CMP_IDENT, do_cmp_ident},
	{CSP_CMP_ROUTE_SET_V1, do_cmp_route_set_v1},
	{CSP_CMP_ROUTE_SET_V2, do_cmp_route_set_v2},
	{CSP_CMP_IF_STATS, do_cmp_if_stats},
	{CSP_CMP_PEEK, do_cmp_peek},
	{CSP_CMP_POKE, do_cmp_poke},
	{CSP_CMP_PEEK_V2, do_cmp_peek_v2},
	{CSP_CMP_POKE_V2, do_cmp_poke_v2},
	{CSP_CMP_CLOCK, do_cmp_clock},
};

/* CSP Management Protocol handler */
static int csp_cmp_handler(csp_packet_t * packet) {

	if (csp_cmp_check_len(packet, sizeof(struct csp_cmp_header)) != CSP_ERR_NONE) {
		return CSP_ERR_INVAL;
	}

	struct csp_cmp_header * cmp = (struct csp_cmp_header *)packet->data;

	/* Ignore everything but requests */
	if (cmp->type != CSP_CMP_REQUEST)
		return CSP_ERR_INVAL;

	for (size_t i = 0; i < (sizeof(csp_cmp_services) / sizeof(csp_cmp_services[0])); i++) {
		if (cmp->code != csp_cmp_services[i].code) {
			continue;
		}

		int ret = csp_cmp_services[i].handler(packet);
		if (ret == CSP_ERR_NONE) {
			cmp->type = CSP_CMP_REPLY;
		}
		return ret;
	}

	return CSP_ERR_INVAL;
}

void csp_service_handler(csp_packet_t * packet) {

	switch (packet->id.dport) {

		case CSP_CMP:
			/* Pass to CMP handler */
			if (csp_cmp_handler(packet) != CSP_ERR_NONE) {
				csp_buffer_free(packet);
				return;
			}
			break;

		case CSP_PING:
			/* A ping means, just echo the packet, so no changes */
			break;

		case CSP_PS: {
			packet->length = csp_ps_hook(packet);
			if (packet->length == 0) {
				csp_buffer_free(packet);
			}
			break;
		}

		case CSP_MEMFREE: {

			uint32_t total = 0;
			total = csp_memfree_hook();
			
			total = htobe32(total);
			memcpy(packet->data, &total, sizeof(total));
			packet->length = sizeof(total);

			break;
		}

		case CSP_REBOOT: {
			uint32_t magic_word;
			memcpy(&magic_word, packet->data, sizeof(magic_word));

			magic_word = be32toh(magic_word);

			/* If the magic word is valid, reboot */
			if (magic_word == CSP_REBOOT_MAGIC) {
				csp_reboot_hook();
			} else if (magic_word == CSP_REBOOT_SHUTDOWN_MAGIC) {
				csp_shutdown_hook();
			}

			csp_buffer_free(packet);
			return;
		}

		case CSP_BUF_FREE: {
			uint32_t size = csp_buffer_remaining();
			size = htobe32(size);
			memcpy(packet->data, &size, sizeof(size));
			packet->length = sizeof(size);
			break;
		}

		case CSP_UPTIME: {
			uint32_t time = csp_get_s();
			time = htobe32(time);
			memcpy(packet->data, &time, sizeof(time));
			packet->length = sizeof(time);
			break;
		}

		default:
			csp_buffer_free(packet);
			return;
	}

	if (packet != NULL) {
		csp_sendto_reply(packet, packet, CSP_O_SAME);
	}
}
