#include <csp/csp_types.h>
#include <inttypes.h>

__attribute__((weak)) void csp_output_hook(csp_id_t idout, csp_packet_t * packet, csp_iface_t * iface, uint16_t via, int from_me);
__attribute__((weak)) void csp_input_hook(csp_iface_t * iface, csp_packet_t * packet);

__attribute__((weak)) void csp_reboot_hook(void);
__attribute__((weak)) void csp_shutdown_hook(void);

__attribute__((weak)) uint32_t csp_memfree_hook(void);

/** Implement these, if you use csp_if_tun */
__attribute__((weak)) int csp_crypto_decrypt(uint8_t * ciphertext_in, uint8_t ciphertext_len, uint8_t * msg_out); // Returns -1 for failure, length if ok
__attribute__((weak)) int csp_crypto_encrypt(uint8_t * msg_begin, uint8_t msg_len, uint8_t * ciphertext_out);     // Returns length of encrypted data
