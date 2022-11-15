#include <csp/interfaces/csp_if_tun.h>
#include <csp/csp.h>
#include <csp/csp_id.h>
#include <csp/csp_hooks.h>
#include "csp_macro.h"

__weak int csp_crypto_decrypt(uint8_t * ciphertext_in, uint8_t ciphertext_len, uint8_t * msg_out) {
	return -1;
}

__weak int csp_crypto_encrypt(uint8_t * msg_begin, uint8_t msg_len, uint8_t * ciphertext_out) {
	return -1;
}

static int csp_if_tun_tx(csp_iface_t * iface, uint16_t via, csp_packet_t * packet, int from_me) {

	csp_if_tun_conf_t * ifconf = iface->driver_data;

	/* Allocate new frame */
	csp_packet_t * new_packet = csp_buffer_get_always();
	if (new_packet == NULL) {
		csp_buffer_free(packet);
		return CSP_ERR_NONE;
	}

	if (packet->id.dst == ifconf->tun_src) {

		/**
		 * Incomming tunnel packet
		 */
		//csp_hex_dump("incoming packet", packet->data, packet->length);

		csp_id_setup_rx(new_packet);

#if 1
		int length = csp_crypto_decrypt(packet->data, packet->length, new_packet->frame_begin);
		if (length < 0) {
			csp_buffer_free(new_packet);
			csp_buffer_free(packet);
			iface->rx_error++;
			return CSP_ERR_NONE;
		} else {
			new_packet->frame_length = length;
		}
#else
		/* Decapsulate */
		memcpy(new_packet->frame_begin, packet->data, packet->length);
		new_packet->frame_length = packet->length;
#endif

		/* Now free old packet */
		csp_buffer_free(packet);

		//csp_hex_dump("new frame", new_packet->frame_begin, new_packet->frame_length + 16);

		csp_id_strip(new_packet);

		//csp_hex_dump("new packet", new_packet->data, new_packet->length);

		/* Send new packet */
		csp_qfifo_write(new_packet, iface, NULL);

	} else {

		/**
		 * Outgoing tunnel packet
		 */

		//csp_hex_dump("packet", packet->data, packet->length);

		/* Apply CSP header */
		csp_id_prepend(packet);

		//csp_hex_dump("frame", packet->frame_begin, packet->frame_length);

		/* Create tunnel header */
		new_packet->id.dst = ifconf->tun_dst;
		new_packet->id.src = ifconf->tun_src;
		new_packet->id.sport = 0;
		new_packet->id.dport = 0;
		new_packet->id.pri = packet->id.pri;
		new_packet->length = packet->frame_length;

#if 1
		/* Encrypt */
		new_packet->length = csp_crypto_encrypt(packet->frame_begin, packet->frame_length, new_packet->data);
#else
		/* Encapsulate */
		memcpy(new_packet->data, packet->frame_begin, packet->frame_length);
#endif

		/* Free old packet */
		csp_buffer_free(packet);

		//csp_hex_dump("new packet", new_packet->data, new_packet->length);

		/* Apply CSP header */
		csp_id_prepend(new_packet);

		//csp_hex_dump("new frame", new_packet->frame_begin, new_packet->frame_length);

		/* Send new packet */
		csp_qfifo_write(new_packet, iface, NULL);

	}

	return CSP_ERR_NONE;

}

void csp_if_tun_init(csp_iface_t * iface, csp_if_tun_conf_t * ifconf) {

	iface->driver_data = ifconf;

	/* Regsiter interface */
	iface->name = "TUN",
	iface->nexthop = csp_if_tun_tx,
	csp_iflist_add(iface);

}

