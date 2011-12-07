/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2011 GomSpace ApS (http://www.gomspace.com)
Copyright (C) 2011 AAUSAT3 Project (http://aausat3.space.aau.dk) 

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

#include <stdio.h>
#include <string.h>
#include <stdint.h>

/* CSP includes */
#include <csp/csp.h>
#include <csp/csp_cmp.h>
#include <csp/csp_endian.h>
#include <csp/csp_platform.h>

#include "arch/csp_time.h"
#include "arch/csp_malloc.h"
#include "arch/csp_system.h"

/* CSP Management Protocol handler */
int csp_cmp_handler(csp_conn_t * conn, csp_packet_t * packet) {

	struct csp_cmp_message * cmp = (struct csp_cmp_message *) packet->data;

	/* Ignore everything but requests */
	if (cmp->type != CSP_CMP_REQUEST)
		return CSP_ERR_INVAL;

	switch (cmp->code) {

	case CSP_CMP_IDENT:
		cmp->type = CSP_CMP_REPLY;

		/* Copy revision */
		strncpy(cmp->ident.revision, GIT_REV, CSP_CMP_IDENT_REV_LEN);
		cmp->ident.revision[CSP_CMP_IDENT_REV_LEN - 1] = '\0';

		/* Copy compilation date */
		strncpy(cmp->ident.date, __DATE__, CSP_CMP_IDENT_DATE_LEN);
		cmp->ident.date[CSP_CMP_IDENT_DATE_LEN - 1] = '\0';

		/* Copy compilation time */
		strncpy(cmp->ident.time, __TIME__, CSP_CMP_IDENT_TIME_LEN);
		cmp->ident.time[CSP_CMP_IDENT_TIME_LEN - 1] = '\0';

		/* Copy hostname */
		strncpy(cmp->ident.hostname, csp_get_hostname(), CSP_HOSTNAME_LEN);
		cmp->ident.hostname[CSP_HOSTNAME_LEN - 1] = '\0';

		/* Copy model name */
		strncpy(cmp->ident.model, csp_get_model(), CSP_MODEL_LEN);
		cmp->ident.model[CSP_MODEL_LEN - 1] = '\0';

		packet->length = CMP_SIZE(ident);
		
		break;
	
	default:
		return CSP_ERR_INVAL;
	}

	return CSP_ERR_NONE;
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
		csp_log_info("SERVICE: Ping received\r\n");
		break;

	case CSP_PS: {
		csp_sys_tasklist((char *)packet->data);
		packet->length = strlen((char *)packet->data);
		packet->data[packet->length] = '\0';
		packet->length++;
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

		/* If the magic word is invalid, return */
		if (magic_word != 0x80078007) {
			csp_buffer_free(packet);
			return;
		}

		/* Otherwise Reboot */
		csp_sys_reboot();
		
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
		uint32_t time = csp_get_s();
		time = csp_hton32(time);
		memcpy(packet->data, &time, sizeof(time));
		packet->length = sizeof(time);
		break;
	}

	default:
		csp_buffer_free(packet);
		return;
	}

	if (!csp_send(conn, packet, 0))
		csp_buffer_free(packet);

}
