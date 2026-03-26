

#include <csp/interfaces/csp_if_rf.h>
#include <csp/csp_rtable.h>
#include <csp/csp_buffer.h>


static int csp_rf_tx(csp_iface_t * iface, uint16_t via, csp_packet_t * packet, int from_me);


int csp_rf_add_interface(csp_iface_t * iface)
{
 
	if ((iface == NULL) || (iface->name == NULL) || (iface->interface_data == NULL)) {
		return CSP_ERR_INVAL;
	}

	csp_rf_interface_data_t * ifdata = iface->interface_data;
	if (ifdata->tx_func == NULL) {
		return CSP_ERR_INVAL;
	}

	ifdata->cfp_packet_counter = 0;

	iface->nexthop = csp_rf_tx;

	csp_iflist_add(iface);

	return CSP_ERR_NONE;

}


int csp_rf_remove_interface(csp_iface_t * iface)
{
    if (iface == NULL) {
        return CSP_ERR_INVAL;
	}

	csp_iflist_remove(iface);

	return CSP_ERR_NONE;
}

int csp_rf_tx(csp_iface_t * iface, uint16_t via, csp_packet_t * packet, int from_me)
{
	csp_rf_interface_data_t * ifdata = iface->interface_data;

	/* Figure out destination node based on routing entry */
	if ( via != CSP_NO_VIA_ADDRESS )
        packet->id.dst = via;
    
    /* Loopback */
	if (packet->id.dst == iface->addr) {
		csp_qfifo_write(packet, iface, NULL);
		return CSP_ERR_NONE;
	}

	/* Send frame */
	if ((ifdata->tx_func)( packet ) != CSP_ERR_NONE) {
		iface->tx_error++;
		/* Does not free on return */
		return CSP_ERR_DRIVER;
	}

	csp_buffer_free(packet);

	return CSP_ERR_NONE;
}