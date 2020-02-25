#!/usr/bin/python3

# libcsp must be build with at least these options to run this example server:
# ./waf distclean configure build --enable-bindings --enable-crc32 --enable-rdp --enable-if-zmq
#                                 --enable-can-socketcan --enable-examples
# Can be run from root of libcsp like this:
# LD_LIBRARY_PATH=build PYTHONPATH=bindings/python:build python3 examples/python_bindings_example_server.py
#

import os
import time
import sys
import libcsp
import threading


def csp_server():
    sock = libcsp.socket()
    libcsp.bind(sock, libcsp.CSP_ANY)
    libcsp.listen(sock, 5)
    while True:
        # wait for incoming connection
        conn = libcsp.accept(sock, libcsp.CSP_MAX_TIMEOUT)
        if not conn:
            continue

        print ("connection: source=%i:%i, dest=%i:%i" % (libcsp.conn_src(conn),
                                                         libcsp.conn_sport(conn),
                                                         libcsp.conn_dst(conn),
                                                         libcsp.conn_dport(conn)))

        while True:
            # Read all packets on the connection
            packet = libcsp.read(conn, 100)
            if packet is None:
                break

            if libcsp.conn_dport(conn) == 10:
                # print request
                data = bytearray(libcsp.packet_get_data(packet))
                length = libcsp.packet_get_length(packet)
                print ("got packet, len=" + str(length) + ", data=" + ''.join('{:02x}'.format(x) for x in data))
                # send reply
                data[0] = data[0] + 1
                reply = libcsp.buffer_get(1)
                libcsp.packet_set_data(reply, data)
                libcsp.sendto_reply(packet, reply, libcsp.CSP_O_NONE)

            else:
                # pass request on to service handler
                libcsp.service_handler(conn, packet)


if __name__ == "__main__":

    # init csp
    libcsp.init(27, "test_service", "bindings", "1.2.3", 10, 300)
    libcsp.zmqhub_init(27, "localhost")
    libcsp.rtable_set(0, 0, "ZMQHUB")
    libcsp.route_start_task()

    print("Hostname: %s" % libcsp.get_hostname())
    print("Model:    %s" % libcsp.get_model())
    print("Revision: %s" % libcsp.get_revision())

    print("Routes:")
    libcsp.print_routes()

    # start CSP server
    threading.Thread(target=csp_server).start()
