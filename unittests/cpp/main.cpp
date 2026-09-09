#include <csp/csp.h>
#include <csp/drivers/can_socketcan.h>
#include <csp/drivers/eth_linux.h>

decltype(&csp_init) volatile csp_init_symbol = &csp_init;
decltype(&csp_get_conf) volatile csp_get_conf_symbol = &csp_get_conf;
decltype(&csp_eth_init) volatile csp_eth_init_symbol = &csp_eth_init;

static int can_tx(void *, uint32_t, const uint8_t *, uint8_t, const csp_packet_t *) {
	return CSP_ERR_NONE;
}

int main() {
	csp_init_symbol();
	if (csp_get_conf_symbol() == nullptr || csp_eth_init_symbol == nullptr) {
		return 1;
	}

	csp_can_interface_data_t can_data{};
	can_data.cfp_packet_counter.store(42);
	can_data.tx_func = can_tx;
	csp_iface_t iface{};
	iface.name = "CPP_CAN";
	iface.interface_data = &can_data;
	if (csp_can_add_interface(&iface) != CSP_ERR_NONE) {
		return 1;
	}
	const bool initialized = can_data.cfp_packet_counter.load() == 0;
	return (csp_can_remove_interface(&iface) != CSP_ERR_NONE || !initialized);
}
