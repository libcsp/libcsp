#!/usr/bin/python

# libcsp must be build with at least these options to run this example client:
# ./waf distclean configure build --enable-bindings --enable-crc32 --enable-rdp --enable-if-zmq --with-driver-usart=linux --enable-if-kiss --enable-xtea --enable-if-can --enable-can-socketcan --enable-hmac --enable-examples

# Can be run from root of libcsp like this:
# LD_LIBRARY_PATH=build PYTHONPATH=bindings/python:build python examples/python_bindings_example_client.py
#

import os
import time
import libcsp as csp


if __name__ == "__main__":

    csp.buffer_init(10, 300)
    csp.init(28)
    csp.zmqhub_init(28, "localhost")
    csp.rtable_set(27, 5, "ZMQHUB")
    csp.route_start_task()

    ## allow router task startup
    time.sleep(1)

    ## cmp_ident
    (rc, host, model, rev, date, time) = csp.cmp_ident(27)
    if rc == csp.CSP_ERR_NONE:
        print (host, model, rev, date, time)
    else:
        print ("error in cmp_ident, rc=%i" % (rc))

    ## transaction
    outbuf = bytearray().fromhex('01')
    inbuf = bytearray(1)
    print ("using csp_transaction to send a single byte")
    if csp.transaction(0, 27, 10, 1000, outbuf, inbuf) < 1:
        print ("csp_transaction failed")
    else:
        print ("got reply, data=" + ''.join('{:02x}'.format(x) for x in inbuf))


