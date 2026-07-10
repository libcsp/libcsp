#include <csp/csp.h>

#include <string.h>

#include <csp/csp_hooks.h>
#include <endian.h>
#include <csp/arch/csp_time.h>

#include "cmp/csp_cmp_internal.h"


static void set_u32_reply(csp_packet_t * packet, uint32_t value) {

	const uint32_t value_be = htobe32(value);

	memcpy(packet->data, &value_be, sizeof(value_be));
	packet->length = sizeof(value_be);
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
				return;
			}
			break;
		}

		case CSP_MEMFREE: {
			set_u32_reply(packet, csp_memfree_hook());
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
			set_u32_reply(packet, (uint32_t)csp_buffer_remaining());
			break;
		}

		case CSP_UPTIME: {
			set_u32_reply(packet, csp_get_s());
			break;
		}

		default:
			csp_buffer_free(packet);
			return;
	}

	csp_sendto_reply(packet, packet, CSP_O_SAME);
}
