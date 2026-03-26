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


def get_options():
    parser = argparse.ArgumentParser(description="Parses command.")
    parser.add_argument("-a", "--address", type=int, default=10, help="Local CSP address")
    parser.add_argument("-c", "--can", help="Add CAN interface")
    parser.add_argument("-k", "--kiss", help="Add KISS interface")
    parser.add_argument("-z", "--zmq", help="Add ZMQ interface")
    parser.add_argument("-s", "--server-address", type=int, default=27, help="Server address")
    parser.add_argument("-R", "--routing-table", help="Routing table")
    return parser.parse_args(sys.argv[1:])


if __name__ == "__main__":

    options = get_options()

    #initialize libcsp with params:
        # options.address - CSP address of the system (default=1)
        # "host"          - Host name, returned by CSP identity requests
        # "model"         - Model, returned by CSP identity requests
        # "1.2.3"         - Revision, returned by CSP identity requests
    # See "include\csp\csp.h" - lines 42-80 for more detail
    # See "src\bindings\python\pycsp.c" - lines 128-156 for more detail
    libcsp.init("host", "model", "1.2.3")

    if options.can:
        # add CAN interface
        libcsp.can_socketcan_init(options.can)                  
   
    if options.zmq: 
        # add ZMQ interface - (address, host)
        # creates publish and subrcribe endpoints from the host        
        libcsp.zmqhub_init(options.address, options.zmq)        
        
        # Format: \<address\>[/mask] \<interface\> [via][, next entry]
        # Examples: "0/0 CAN, 8 KISS, 10 I2C 10", same as "0/0 CAN, 8/5 KISS, 10/5 I2C 10"
        libcsp.rtable_load("0/0 ZMQHUB")

    if options.kiss:
        libcsp.kiss_init(options.kiss, options.address)
        libcsp.rtable_load("0/0 KISS")

    if options.routing_table:
        # same format/use as line above
        libcsp.rtable_load(options.routing_table)

    # Parameters: {priority} - 0 (critical), 1 (high), 2 (norm), 3 (low) ---- default=2
    # Start the router task - creates routing thread
    libcsp.route_start_task()
    time.sleep(0.2)  # allow router task startup

    print("Connections:")
    # Prints connections format: 
    # [state 1 or 0 (open/closed)] [5-bit src address] [5-bit dest address] [6-bit dest port] [6 bit src port] [socket to be woken when packet is ready]
    libcsp.print_connections()

    print("Interfaces:")
    # Prints interfaces format:
    libcsp.print_interfaces()

    print("Routes:")
    # Prints route table format: 
    # [address] [netmask] [interface name] optional([via])
    libcsp.print_routes()

    # Parameters: {node} - address of subsystem, optional:{timeout ms (default=1000)}
    # CSP Management Protocol (CMP)
    # CMP identification request
    print("CMP ident:", libcsp.cmp_ident(options.server_address))

    # Parameters: {address of subsystem}, optional:{timeout ms (default=1000)}, optional:{data size in bytes (default=10)}, optional:{connection options as bit flags (see "src\csp_conn.c")} 
    # Ping message
    print("Ping: %d ms" % libcsp.ping(options.server_address))


    # *** more services can be found at "src\csp_services.c" *** #


    # transaction
    outbuf = bytearray().fromhex('01')
    inbuf = bytearray(1)
    print ("Exchange data with server using csp_transaction ...")

    # Perform an entire request & reply transaction w/ params:
        # 0                       - priority
        # options.server_address  - dest address
        # 10                      - dest port 
        # 1000                    - timeout ms
        # outbuf                  - outgoing data (request)
        # inbuf                   - buffer provided for recieving data (reply)      
    libcsp.transaction(0, options.server_address, 10, 1000, outbuf, inbuf)
    print ("  got reply from server [%s]" % (''.join('{:02x}'.format(x) for x in inbuf)))
