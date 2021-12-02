

#include <csp/csp.h>

#include <stdio.h>
#ifdef __GNUC__
#ifndef alloca
#define alloca __builtin_alloca
#endif
#else
#include <alloca.h>
#endif
#include <string.h>

#include <csp/csp_cmp.h>
#include <endian.h>
#include <csp/csp_types.h>
#include <csp/csp_rtable.h>
#include <csp/csp_id.h>
#include <csp/arch/csp_time.h>
#include <csp/arch/csp_clock.h>
#include <csp/arch/csp_system.h>

#define CSP_RPS_MTU 196

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
#endif

void csp_cmp_set_memcpy(csp_memcpy_fnc_t fnc) {
	csp_cmp_memcpy_fnc = fnc;
}

static int do_cmp_ident(struct csp_cmp_message * cmp) {

	/* Copy revision */
	strncpy(cmp->ident.revision, csp_conf.revision, CSP_CMP_IDENT_REV_LEN);
	cmp->ident.revision[CSP_CMP_IDENT_REV_LEN - 1] = '\0';

	/* Copy compilation date */
	strncpy(cmp->ident.date, __DATE__, CSP_CMP_IDENT_DATE_LEN);
	cmp->ident.date[CSP_CMP_IDENT_DATE_LEN - 1] = '\0';

	/* Copy compilation time */
	strncpy(cmp->ident.time, __TIME__, CSP_CMP_IDENT_TIME_LEN);
	cmp->ident.time[CSP_CMP_IDENT_TIME_LEN - 1] = '\0';

	/* Copy hostname */
	strncpy(cmp->ident.hostname, csp_conf.hostname, CSP_HOSTNAME_LEN);
	cmp->ident.hostname[CSP_HOSTNAME_LEN - 1] = '\0';

	/* Copy model name */
	strncpy(cmp->ident.model, csp_conf.model, CSP_MODEL_LEN);
	cmp->ident.model[CSP_MODEL_LEN - 1] = '\0';

	return CSP_ERR_NONE;
}

static int do_cmp_route_set_v1(struct csp_cmp_message * cmp) {

	csp_iface_t * ifc = csp_iflist_get_by_name(cmp->route_set_v1.interface);
	if (ifc == NULL) {
		return CSP_ERR_INVAL;
	}

	if (csp_rtable_set(cmp->route_set_v1.dest_node, csp_id_get_host_bits(), ifc, cmp->route_set_v1.next_hop_via) != CSP_ERR_NONE) {
		return CSP_ERR_INVAL;
	}

	return CSP_ERR_NONE;
}

static int do_cmp_route_set_v2(struct csp_cmp_message * cmp) {

	csp_iface_t * ifc = csp_iflist_get_by_name(cmp->route_set_v2.interface);
	if (ifc == NULL) {
		return CSP_ERR_INVAL;
	}

	if (csp_rtable_set(be16toh(cmp->route_set_v2.dest_node), be16toh(cmp->route_set_v2.netmask), ifc, be16toh(cmp->route_set_v2.next_hop_via)) != CSP_ERR_NONE) {
		return CSP_ERR_INVAL;
	}

	return CSP_ERR_NONE;
}

static int do_cmp_if_stats(struct csp_cmp_message * cmp) {

	csp_iface_t * ifc = csp_iflist_get_by_name(cmp->if_stats.interface);
	if (ifc == NULL)
		return CSP_ERR_INVAL;

	cmp->if_stats.tx = htobe32(ifc->tx);
	cmp->if_stats.rx = htobe32(ifc->rx);
	cmp->if_stats.tx_error = htobe32(ifc->tx_error);
	cmp->if_stats.rx_error = htobe32(ifc->rx_error);
	cmp->if_stats.drop = htobe32(ifc->drop);
	cmp->if_stats.autherr = htobe32(ifc->autherr);
	cmp->if_stats.frame = htobe32(ifc->frame);
	cmp->if_stats.txbytes = htobe32(ifc->txbytes);
	cmp->if_stats.rxbytes = htobe32(ifc->rxbytes);
	cmp->if_stats.irq = htobe32(ifc->irq);

	return CSP_ERR_NONE;
}

static int do_cmp_peek(struct csp_cmp_message * cmp) {

	cmp->peek.addr = htobe32(cmp->peek.addr);
	if (cmp->peek.len > CSP_CMP_PEEK_MAX_LEN)
		return CSP_ERR_INVAL;

	/* Dangerous, you better know what you are doing */
	csp_cmp_memcpy_fnc((csp_memptr_t)(uintptr_t)cmp->peek.data, (csp_memptr_t)(uintptr_t)cmp->peek.addr, cmp->peek.len);

	return CSP_ERR_NONE;
}

static int do_cmp_poke(struct csp_cmp_message * cmp) {

	cmp->poke.addr = htobe32(cmp->poke.addr);
	if (cmp->poke.len > CSP_CMP_POKE_MAX_LEN)
		return CSP_ERR_INVAL;

	/* Extremely dangerous, you better know what you are doing */
	csp_cmp_memcpy_fnc((csp_memptr_t)(uintptr_t)cmp->poke.addr, (csp_memptr_t)(uintptr_t)cmp->poke.data, cmp->poke.len);

	return CSP_ERR_NONE;
}

static int do_cmp_clock(struct csp_cmp_message * cmp) {

	csp_timestamp_t clock;
	clock.tv_sec = be32toh(cmp->clock.tv_sec);
	clock.tv_nsec = be32toh(cmp->clock.tv_nsec);

	int res = CSP_ERR_NONE;
	if (clock.tv_sec != 0) {
		// set time
		res = csp_clock_set_time(&clock);
		if (res != CSP_ERR_NONE) {
			//csp_print("csp_clock_set_time(sec=%" PRIu32 ", nsec=%" PRIu32 ") failed, error: %d\n", clock.tv_sec, clock.tv_nsec, res);
		}
	}

	csp_clock_get_time(&clock);

	cmp->clock.tv_sec = htobe32(clock.tv_sec);
	cmp->clock.tv_nsec = htobe32(clock.tv_nsec);

	return res;
}

/* CSP Management Protocol handler */
static int csp_cmp_handler(csp_packet_t * packet) {

	int ret = CSP_ERR_INVAL;
	struct csp_cmp_message * cmp = (struct csp_cmp_message *)packet->data;

	/* Ignore everything but requests */
	if (cmp->type != CSP_CMP_REQUEST)
		return ret;

	switch (cmp->code) {
		case CSP_CMP_IDENT:
			ret = do_cmp_ident(cmp);
			packet->length = CMP_SIZE(ident);
			break;

		case CSP_CMP_ROUTE_SET_V1:
			ret = do_cmp_route_set_v1(cmp);
			packet->length = CMP_SIZE(route_set_v1);
			break;

		case CSP_CMP_ROUTE_SET_V2:
			ret = do_cmp_route_set_v2(cmp);
			packet->length = CMP_SIZE(route_set_v2);
			break;

		case CSP_CMP_IF_STATS:
			ret = do_cmp_if_stats(cmp);
			packet->length = CMP_SIZE(if_stats);
			break;

		case CSP_CMP_PEEK:
			ret = do_cmp_peek(cmp);
			break;

		case CSP_CMP_POKE:
			ret = do_cmp_poke(cmp);
			break;

		case CSP_CMP_CLOCK:
			ret = do_cmp_clock(cmp);
			break;

		default:
			ret = CSP_ERR_INVAL;
			break;
	}

	cmp->type = CSP_CMP_REPLY;

	return ret;
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
			/* Sanity check on request */
			if ((packet->length != 1) || (packet->data[0] != 0x55)) {
				/* Sanity check failed */
				csp_buffer_free(packet);
				/* Clear the packet, it has been freed */
				packet = NULL;
				break;
			}
			/* Start by allocating just the right amount of memory */
			int task_list_size = csp_sys_tasklist_size();
			char * pslist = alloca(task_list_size);
			/* Check for malloc fail */
			if (pslist == NULL) {
				/* Send out the data */
				strcpy((char *)packet->data, "Not enough memory");
				packet->length = strlen((char *)packet->data);
				/* Break and let the default handling send packet */
				break;
			}

			/* Retrieve the tasklist */
			csp_sys_tasklist(pslist);
			int pslen = strnlen(pslist, task_list_size);

			/* Split the potentially very long string into packets */
			int i = 0;
			while (i < pslen) {

				/* Allocate packet buffer, if need be */
				csp_packet_t * rpacket = csp_buffer_get(CSP_RPS_MTU);
				if (rpacket == NULL)
					break;

				/* Calculate length, either full MTU or the remainder */
				rpacket->length = (pslen - i > CSP_RPS_MTU) ? CSP_RPS_MTU : (pslen - i);

				/* Send out the data */
				memcpy(rpacket->data, &pslist[i], rpacket->length);
				i += rpacket->length;
				csp_sendto_reply(packet, rpacket, CSP_O_SAME);

				/* Clear the packet reference when sent */
			}
			csp_buffer_free(packet);
			packet = NULL;
			break;
		}

		case CSP_MEMFREE: {
			uint32_t total = csp_sys_memfree();

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
				csp_sys_reboot();
			} else if (magic_word == CSP_REBOOT_SHUTDOWN_MAGIC) {
				csp_sys_shutdown();
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
