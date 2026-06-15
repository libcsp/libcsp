#include <csp/csp.h>

#include <string.h>

#include <csp/csp_hooks.h>
#include <endian.h>
#include <csp/arch/csp_time.h>

#include "cmp/csp_cmp_internal.h"

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
