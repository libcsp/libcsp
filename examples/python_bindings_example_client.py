#!/usr/bin/python3

# Build required code:
# $ ./examples/buildall.py
#
# Start zmqproxy (only one instance)
# $ ./build/zmqproxy
#
# Run client against server using ZMQ:
# $ LD_LIBRARY_PATH=build PYTHONPATH=build python3 examples/python_bindings_example_client.py -z localhost
#

import os
import time
import sys
import argparse

import libcsp_py3 as libcsp


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

    libcsp.init(options.address, "host", "model", "1.2.3", 10, 300)

    if options.can:
        libcsp.can_socketcan_init(options.can)
    if options.zmq:
        libcsp.zmqhub_init(options.address, options.zmq)
        libcsp.rtable_load("0/0 ZMQHUB")
    if options.routing_table:
        libcsp.rtable_load(options.routing_table)

    libcsp.route_start_task()
    time.sleep(0.2)  # allow router task startup

    print("Connections:")
    libcsp.print_connections()

    print("Routes:")
    libcsp.print_routes()

    print("CMP ident:", libcsp.cmp_ident(options.server_address))

    print("Ping: %d mS" % libcsp.ping(options.server_address))

    # transaction
    outbuf = bytearray().fromhex('01')
    inbuf = bytearray(1)
    print ("Exchange data with server using csp_transaction ...")
    libcsp.transaction(0, options.server_address, 10, 1000, outbuf, inbuf)
    print ("  got reply from server [%s]" % (''.join('{:02x}'.format(x) for x in inbuf)))
