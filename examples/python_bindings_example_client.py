#!/usr/bin/python3

# libcsp must be build with at least these options to run this example client:
# ./waf distclean configure build --enable-bindings --enable-crc32 --enable-rdp --enable-if-zmq
#                                 --with-driver-usart=linux --enable-if-kiss --enable-xtea --enable-if-can
#                                 --enable-can-socketcan --enable-hmac --enable-examples
# Can be run from root of libcsp like this:
# LD_LIBRARY_PATH=build PYTHONPATH=bindings/python:build python3 examples/python_bindings_example_client.py
#

import os
import time
import libcsp as csp
import sys
import argparse


def getOptions():
    parser = argparse.ArgumentParser(description="Parses command.")
    parser.add_argument("-a", "--address", type=int, default=10, help="Local CSP address")
    parser.add_argument("-c", "--can", help="Add CAN interface")
    parser.add_argument("-z", "--zmq", help="Add ZMQ interface")
    parser.add_argument("-s", "--server-address", type=int, default=27, help="Server address")
    parser.add_argument("-R", "--routing-table", help="Routing table")
    return parser.parse_args(sys.argv[1:])


if __name__ == "__main__":

    options = getOptions()

    csp.init(options.address, "host", "model", "1.2.3", 10, 300)

    if options.can:
        csp.can_socketcan_init(options.can)
    if options.zmq:
        csp.zmqhub_init(options.address, options.zmq)
    if options.routing_table:
        csp.rtable_load(options.routing_table)

    csp.route_start_task()
    time.sleep(0.2)  # allow router task startup

    print("Connections:")
    csp.print_connections()

    print("Routes:")
    csp.print_routes()

    print("CMP ident:", csp.cmp_ident(options.server_address))

    print("Ping: %d mS" % csp.ping(options.server_address))

    # transaction
    outbuf = bytearray().fromhex('01')
    inbuf = bytearray(1)
    print ("Exchange data with server using csp_transaction ...")
    csp.transaction(0, options.server_address, 10, 1000, outbuf, inbuf)
    print ("  got reply from server [%s]" % (''.join('{:02x}'.format(x) for x in inbuf)))
