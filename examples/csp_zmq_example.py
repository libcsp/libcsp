#!/usr/bin/python
import sys
from csp import lib as csp
from csp import ffi

NODE_ID = 8

csp.csp_set_hostname("Test");
csp.csp_set_model("Python bindings");
csp.csp_buffer_init(400, 512);
csp.csp_init(NODE_ID);
csp.csp_zmqhub_init(NODE_ID, 'localhost')

print csp.csp_iflist_print()
print csp.csp_rtable_print()
print "--------"
print 

csp.csp_rtable_set(csp.CSP_DEFAULT_ROUTE, csp.CSP_ID_HOST_SIZE, ffi.addressof(csp.csp_if_zmqhub), csp.CSP_NODE_MAC)
csp.csp_route_start_task(1000, 0)

print csp.csp_iflist_print()
print csp.csp_rtable_print()


print csp.csp_ping(10, 1000, 10, 0)

print "------"
print

print csp.csp_iflist_print()
print csp.csp_rtable_print()

