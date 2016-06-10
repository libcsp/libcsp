#!/usr/bin/python
import sys
from csp import lib as csp
from csp import ffi

NODE_ID = 8

csp.csp_init(NODE_ID)
csp.csp_set_hostname("Test - CAN");
csp.csp_set_model("Python bindings - CAN");

csp.csp_buffer_init(400, 512);

ifcname = bytearray("can0")
ifcname_len = len(ifcname)
cfg = ffi.new('struct csp_can_config*')
cfg.ifc = ffi.new('char[]', 'can0')

CSP_CAN_PROMISC = 1
csp.csp_can_init(CSP_CAN_PROMISC, cfg)

csp.csp_rtable_set(csp.CSP_DEFAULT_ROUTE, csp.CSP_ID_HOST_SIZE, ffi.addressof(csp.csp_if_can), csp.CSP_NODE_MAC)
csp.csp_route_start_task(1000, 0)


csp.csp_ping(7, 1000, 10, 0)

raw_input("Press any key to end....")
