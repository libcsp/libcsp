#include <csp/interfaces/csp_if_kiss.h>
#include <csp/csp_debug.h>
#include <csp/csp.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <unistd.h>


typedef struct csp_tcp_conf {
    int port;
    short socket;
    char * address;
} csp_tcp_conf_t;

int csp_tcp_open(const csp_tcp_conf_t *conf) {
    struct sockaddr_in remote = {0};         // all members set to zero
    remote.sin_addr.s_addr = inet_addr(conf->address);
    remote.sin_port = htons(conf->port);     // htons: converts port to big-endian for networking
    remote.sin_family = AF_INET;		     // address family: IPV4

    return connect(
        conf->socket, (struct sockaddr *)&remote, sizeof(struct sockaddr_in)
    );
}

int csp_tcp_send(const csp_tcp_conf_t *conf, void *user_data, int length) {
    struct timeval tv;
    tv.tv_sec = 20;
    tv.tv_usec = 0;

    // if setting the socket options fails
    if(setsockopt(conf->socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(tv)) < 0) {
        printf("timeout!\n");
        return -1;
        // TODO: CSP Error?
    }

    return send(conf->socket, user_data, length, 0);
}

int csp_tcp_receive(const csp_tcp_conf_t *conf, void *receive_data, int length) {
    int retval = -1;

    // timeval {
    //   seconds
    //   useconds
    // }
    struct timeval tv;
    tv.tv_sec = 20;     // 20 second timeout
    tv.tv_usec = 0;

    // Enums from socket-constants.h
    if(setsockopt(conf->socket, SOL_SOCKET, SO_RCVTIMEO,(char *)&tv,sizeof(tv)) < 0)
    {
        printf("timeout!\n");
        return -1;
        // TODO: CSP Error?
    }

    // recv() is a blocking function, which will block
    // the thread until data is received
    retval = recv( conf->socket, receive_data, length, 0);

    //printf("response: %s\n", response);
    return retval;
}