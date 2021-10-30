#!/usr/bin/python3

# Build required code:
# $ ./examples/buildall.py
#
# Start zmqproxy (only one instance)
# $ ./build/zmqproxy
#
# Run server, default enabling ZMQ interface:
# $ LD_LIBRARY_PATH=build PYTHONPATH=build python3 examples/python_bindings_example_server.py
#

import os
import time
import sys
import threading

import libcsp_py3 as libcsp


def csp_server():
    # parameters: {options} - bit flag corresponding to socket options (see "include\csp\csp_types.h" lines 167-180)
    # creates new socket endpoint, returns socket or None
    # libcsp.listen(<~>,<~>) must proceed this function call somewhere
    sock = libcsp.socket()

    # bind port to socket
    # parameters: {socket}, {port} - (libcsp.CSP_ANY=255 meaning 'listen on all ports')
    libcsp.bind(sock, libcsp.CSP_ANY)

    # Set socket to listen for incoming connections
    # parameters: {socket}, {backlog - default=10 (max length of backlog queue, incoming connection need to be "accepted" using libcsp.accept())}
    libcsp.listen(sock, 5)
    while True:
        # wait for incoming connection
        # parameters: {socket} optional:{timeout}
        # returns the connection data or None
        conn = libcsp.accept(sock, libcsp.CSP_MAX_TIMEOUT)
        if not conn:
            continue

        # print connection source address/port and destination address/port
        print ("connection: source=%i:%i, dest=%i:%i" % (libcsp.conn_src(conn),
                                                         libcsp.conn_sport(conn),
                                                         libcsp.conn_dst(conn),
                                                         libcsp.conn_dport(conn)))

        while True:
            # Read incoming packets on the connection
            # parameters: {connection} optional:{timeout (default=500ms)}
            # returns the entire packet
            packet = libcsp.read(conn, 100)
            if packet is None:
                break

            # connection destination port
            if libcsp.conn_dport(conn) == 10:
                # extract the data payload from the packet
                # see "include\csp\csp_types.h" line 215-239 for packet structure
                data = bytearray(libcsp.packet_get_data(packet))
                
                # get length of the data (not the whole packet, just the data length)
                length = libcsp.packet_get_length(packet)
                print ("got packet, len=" + str(length) + ", data=" + ''.join('{:02x}'.format(x) for x in data))
                
                # send back "input data + 1"
                data[0] = data[0] + 1
                
                # free up a buffer to hold the reply
                # parameters: {buffer size (# of 4-byte doublewords)}
                reply = libcsp.buffer_get(1)
               
                # store the data into the reply buffer
                libcsp.packet_set_data(reply, data)
                
                # Send packet as a reply
                # uses the info (address/port) from the original packet to reply
                # parameters:
                    # packet    - the incoming packet (request packet)
                    # reply     - data buffer containing reply
                    # options   - *optional* connection options (see "include\csp\csp_types.h" line 184-195)
                    # timeout   - *optional* in ms (default=1000ms)
                libcsp.sendto_reply(packet, reply, libcsp.CSP_O_NONE)

            else:
                # pass request on to service handler if the given packet is a service-request
                # (ie: destination port is [0-6] see "include\csp\csp_types.h" line 47-55)
                # parameters: {connection} {packet}
                # will handle and send reply packets if necessary
                libcsp.service_handler(conn, packet)


if __name__ == "__main__":

    #initialize libcsp with params:
        # 27              - CSP address of the system (default=1)
        # "test_service"  - Host name, returned by CSP identity requests
        # "bindings"      - Model, returned by CSP identity requests
        # "1.2.3"         - Revision, returned by CSP identity requests
    # See "include\csp\csp.h" - lines 42-80 for more detail
    # See "src\bindings\python\pycsp.c" - lines 128-156 for more detail
    libcsp.init(27, "test_service", "bindings", "1.2.3")
    
    # init zmqhub with parameters: {address (using 255 means all addresses)} {host name/ip}
    # subscribe and publish endpoints are created on the default ports using the {host}
    # subscribe port = 6000, subscribe port = 7000
    libcsp.zmqhub_init(27, "localhost")

    # params: 
        # {address}         - dest address/node
        # {netmask}         - number of bits in netmask
        # {interface name}  - name of interface
        # optional{via}     - associated with address
    libcsp.rtable_set(0, 0, "ZMQHUB")

    # Parameters: {priority} - 0 (critical), 1 (high), 2 (norm), 3 (low) ---- default=2
    # Start the router task - creates routing thread
    libcsp.route_start_task()

    # get libscp object value
    print("Hostname: %s" % libcsp.get_hostname())
    print("Model:    %s" % libcsp.get_model())
    print("Revision: %s" % libcsp.get_revision())

    print("Routes:")
    libcsp.print_routes()

    # start CSP server in a thread
    threading.Thread(target=csp_server).start()
