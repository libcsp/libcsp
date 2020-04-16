/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2012 GomSpace ApS (http://www.gomspace.com)
Copyright (C) 2012 AAUSAT3 Project (http://aausat3.space.aau.dk) 

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <csp/csp.h>

#include <stdio.h>
#include <string.h>

#include <csp/csp_cmp.h>
#include <csp/csp_endian.h>
#include <csp/csp_platform.h>
#include <csp/csp_rtable.h>
#include <csp/arch/csp_time.h>
#include <csp/arch/csp_clock.h>
#include <csp/arch/csp_malloc.h>
#include <csp/arch/csp_system.h>

#include "csp_init.h"

#define CSP_RPS_MTU	196

/**
 * The CSP CMP mempy function is used to, override the function used to
 * read/write memory by peek and poke.
 */
#ifdef __AVR__
static uint32_t wrap_32bit_memcpy(uint32_t to, const uint32_t from, size_t size) {
	return (uint32_t) (uintptr_t) memcpy((void *) (uintptr_t) to, (const void *) (uintptr_t) from, size);
}
static csp_memcpy_fnc_t csp_cmp_memcpy_fnc = wrap_32bit_memcpy;
#else
static csp_memcpy_fnc_t csp_cmp_memcpy_fnc = (csp_memcpy_fnc_t) memcpy;
#endif


void csp_cmp_set_memcpy(csp_memcpy_fnc_t fnc) {
	csp_cmp_memcpy_fnc = fnc;
}

static int do_cmp_ident(struct csp_cmp_message *cmp) {

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

static int do_cmp_route_set(struct csp_cmp_message *cmp) {

	csp_iface_t *ifc = csp_iflist_get_by_name(cmp->route_set.interface);
	if (ifc == NULL) {
		return CSP_ERR_INVAL;
	}

	if (csp_route_set(cmp->route_set.dest_node, ifc, cmp->route_set.next_hop_via) != CSP_ERR_NONE) {
		return CSP_ERR_INVAL;
	}

	return CSP_ERR_NONE;

}

static int do_cmp_if_stats(struct csp_cmp_message *cmp) {

	csp_iface_t *ifc = csp_iflist_get_by_name(cmp->if_stats.interface);
	if (ifc == NULL)
		return CSP_ERR_INVAL;

	cmp->if_stats.tx =       csp_hton32(ifc->tx);
	cmp->if_stats.rx =       csp_hton32(ifc->rx);
	cmp->if_stats.tx_error = csp_hton32(ifc->tx_error);
	cmp->if_stats.rx_error = csp_hton32(ifc->rx_error);
	cmp->if_stats.drop =     csp_hton32(ifc->drop);
	cmp->if_stats.autherr =  csp_hton32(ifc->autherr);
	cmp->if_stats.frame =    csp_hton32(ifc->frame);
	cmp->if_stats.txbytes =  csp_hton32(ifc->txbytes);
	cmp->if_stats.rxbytes =  csp_hton32(ifc->rxbytes);
	cmp->if_stats.irq = 	 csp_hton32(ifc->irq);

	return CSP_ERR_NONE;
}

static int do_cmp_peek(struct csp_cmp_message *cmp) {

	cmp->peek.addr = csp_hton32(cmp->peek.addr);
	if (cmp->peek.len > CSP_CMP_PEEK_MAX_LEN)
		return CSP_ERR_INVAL;

	/* Dangerous, you better know what you are doing */
	csp_cmp_memcpy_fnc((csp_memptr_t) (uintptr_t) cmp->peek.data, (csp_memptr_t) (uintptr_t) cmp->peek.addr, cmp->peek.len);

	return CSP_ERR_NONE;

}

static int do_cmp_poke(struct csp_cmp_message *cmp) {

	cmp->poke.addr = csp_hton32(cmp->poke.addr);
	if (cmp->poke.len > CSP_CMP_POKE_MAX_LEN)
		return CSP_ERR_INVAL;

	/* Extremely dangerous, you better know what you are doing */
	csp_cmp_memcpy_fnc((csp_memptr_t) (uintptr_t) cmp->poke.addr, (csp_memptr_t) (uintptr_t) cmp->poke.data, cmp->poke.len);

	return CSP_ERR_NONE;

}

static int do_cmp_clock(struct csp_cmp_message *cmp) {

	csp_timestamp_t clock;
	clock.tv_sec = csp_ntoh32(cmp->clock.tv_sec);
	clock.tv_nsec = csp_ntoh32(cmp->clock.tv_nsec);

	int res = CSP_ERR_NONE;
	if (clock.tv_sec != 0) {
		// set time
		res = csp_clock_set_time(&clock);
		if (res != CSP_ERR_NONE) {
			csp_log_warn("csp_clock_set_time(sec=%"PRIu32", nsec=%"PRIu32") failed, error: %d", clock.tv_sec, clock.tv_nsec, res);
		}
	}

	csp_clock_get_time(&clock);

	cmp->clock.tv_sec = csp_hton32(clock.tv_sec);
	cmp->clock.tv_nsec = csp_hton32(clock.tv_nsec);

	return res;

}

/* CSP Management Protocol handler */
static int csp_cmp_handler(csp_conn_t * conn, csp_packet_t * packet) {

	int ret = CSP_ERR_INVAL;
	struct csp_cmp_message * cmp = (struct csp_cmp_message *) packet->data;

	/* Ignore everything but requests */
	if (cmp->type != CSP_CMP_REQUEST)
		return ret;

	switch (cmp->code) {
		case CSP_CMP_IDENT:
			ret = do_cmp_ident(cmp);
			packet->length = CMP_SIZE(ident);
			break;

		case CSP_CMP_ROUTE_SET:
			ret = do_cmp_route_set(cmp);
			packet->length = CMP_SIZE(route_set);
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

void csp_service_handler(csp_conn_t * conn, csp_packet_t * packet) {

	switch (csp_conn_dport(conn)) {

	case CSP_CMP:
		/* Pass to CMP handler */
		if (csp_cmp_handler(conn, packet) != CSP_ERR_NONE) {
			csp_buffer_free(packet);
			return;
		}
		break;

	case CSP_PING:
		/* A ping means, just echo the packet, so no changes */
		csp_log_info("SERVICE: Ping received");
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
		char * pslist = csp_malloc(task_list_size);
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
		while(i < pslen) {

			/* Allocate packet buffer, if need be */
			if (packet == NULL)
				packet = csp_buffer_get(CSP_RPS_MTU);
			if (packet == NULL)
				break;

			/* Calculate length, either full MTU or the remainder */
			packet->length = (pslen - i > CSP_RPS_MTU) ? CSP_RPS_MTU : (pslen - i);

			/* Send out the data */
			memcpy(packet->data, &pslist[i], packet->length);
			i += packet->length;
			if (!csp_send(conn, packet, 0))
				csp_buffer_free(packet);

			/* Clear the packet reference when sent */
			packet = NULL;

		}
		csp_free(pslist);
		break;
	}

	case CSP_MEMFREE: {
		uint32_t total = csp_sys_memfree();

		total = csp_hton32(total);
		memcpy(packet->data, &total, sizeof(total));
		packet->length = sizeof(total);

		break;
	}

	case CSP_REBOOT: {
		uint32_t magic_word;
		memcpy(&magic_word, packet->data, sizeof(magic_word));

		magic_word = csp_ntoh32(magic_word);

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
		size = csp_hton32(size);
		memcpy(packet->data, &size, sizeof(size));
		packet->length = sizeof(size);
		break;
	}

	case CSP_UPTIME: {
		uint32_t time = csp_get_uptime_s();
		time = csp_hton32(time);
		memcpy(packet->data, &time, sizeof(time));
		packet->length = sizeof(time);
		break;
	}

	default:
		csp_buffer_free(packet);
		return;
	}

	if (packet != NULL) {
		if (!csp_send(conn, packet, 0))
			csp_buffer_free(packet);
	}

}
