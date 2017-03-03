#!/usr/bin/python

import sys
import os
import time

import libcsp

if __name__ == "__main__":

    # init csp
    libcsp.csp_buffer_init(10, 300);
    libcsp.csp_init(27);
    libcsp.csp_zmqhub_init(27, "localhost")
    libcsp.csp_rtable_set(7, 5, libcsp.csp_zmqhub_if(), libcsp.CSP_NODE_MAC)
    libcsp.csp_route_start_task(1000, 0)
    time.sleep(1) # allow router startup
    libcsp.csp_rtable_print()

    print "pinging addr 7, rc=" + str(libcsp.csp_ping(7, 5000, 10))

    # start listening for packets...
    sock = libcsp.csp_socket()
    libcsp.csp_bind(sock, 30)
    libcsp.csp_listen(sock, 10)
    while True:
        conn = libcsp.csp_accept(sock, 100)
        if not conn:
            continue

        while True:
            packet = libcsp.csp_read(conn, 100)
            if not packet:
                break

            print "got packet, len=" + str(libcsp.packet_length(packet))

            libcsp.csp_buffer_free(packet)
        libcsp.csp_close(conn)

