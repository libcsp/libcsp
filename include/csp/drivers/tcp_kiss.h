#include <csp/interfaces/csp_if_kiss.h>

#ifndef TCP_KISS_H
#define TCP_KISS_H

typedef struct csp_tcp_conf {
    int port;
    short socket;
    char * address;
} csp_tcp_conf_t;

typedef struct {
	char name[CSP_IFLIST_NAME_MAX + 1];
	csp_iface_t iface;
	csp_kiss_interface_data_t ifdata;
    csp_tcp_conf_t tcp_driver;
} tcp_kiss_context_t;

int csp_tcp_open_and_add_kiss_interface(const csp_tcp_conf_t *conf, const char * ifname, csp_iface_t ** return_iface);

#endif /* TCP_KISS_H */