#!/usr/bin/python

# libcsp must be build with at least these options to run this example server:
# ./waf distclean configure build --enable-bindings --enable-crc32 --enable-rdp --enable-if-zmq --with-driver-usart=linux --enable-if-kiss --enable-xtea --enable-if-can --enable-can-socketcan --enable-hmac --enable-examples

# Can be run from root of libcsp like this:
# LD_LIBRARY_PATH=build PYTHONPATH=bindings/python:build python examples/python_bindings_example_server.py
#

import os
import time
import sys
import libcsp as csp
import subprocess

if __name__ == "__main__":

    # start a zmqproxy to transport messages to and from the client
    zmqp = subprocess.Popen('build/zmqproxy')

    # init csp
    csp.buffer_init(10, 300)
    csp.init(27)
    csp.zmqhub_init(27, "localhost")
    csp.rtable_set(28, 5, "ZMQHUB")
    csp.route_start_task()

    # set identity
    csp.set_hostname("test_service")
    csp.set_model("bindings")
    csp.set_revision("1.2.3")

    # and read it back
    print (csp.get_hostname())
    print (csp.get_model())
    print (csp.get_revision())

    # start listening for packets...
    sock = csp.socket()
    csp.bind(sock, csp.CSP_ANY)
    csp.listen(sock)
    while True:
        conn = csp.accept(sock)
        if not conn:
            continue

        print ("connection: source=%i:%i, dest=%i:%i" % (csp.conn_src(conn),
                                                        csp.conn_sport(conn),
                                                        csp.conn_dst(conn),
                                                        csp.conn_dport(conn)))

        while True:
            packet = csp.read(conn)
            if not packet:
                break

            if csp.conn_dport(conn) == 10:
                data = bytearray(csp.packet_get_data(packet))
                length = csp.packet_get_length(packet)
                print ("got packet, len=" + str(length) + ", data=" + ''.join('{:02x}'.format(x) for x in data))

                data[0] = data[0] + 1
                reply_packet = csp.buffer_get(1)
                if reply_packet:
                    csp.packet_set_data(reply_packet, data)
                    csp.sendto_reply(packet, reply_packet, csp.CSP_O_NONE)

                csp.buffer_free(packet)
            else:
                csp.service_handler(conn, packet)
        csp.close(conn)

