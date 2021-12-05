#include <csp/csp_types.h>
#include <inttypes.h>

/* Hooks for input/output */
__attribute__((weak)) void csp_output_hook(csp_id_t idout, csp_packet_t * packet, csp_iface_t * iface, uint16_t via, int from_me);
__attribute__((weak)) void csp_input_hook(csp_iface_t * iface, csp_packet_t * packet);

__attribute__((weak)) void csp_reboot_hook(void);
__attribute__((weak)) void csp_shutdown_hook(void);