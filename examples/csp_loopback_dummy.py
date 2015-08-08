#!/usr/bin/python3
import sys
from csp import lib as csp
from csp import ffi


PORT = 10
BUF_SIZE = 250
TIMEOUT = 100 #CSP_MAX_DELAY to block
NODE_ID = 1


csp.csp_init(NODE_ID)
csp.csp_buffer_init(10, BUF_SIZE)
csp_if_lo = ffi.new("csp_iface_t *")
csp.csp_rtable_set(csp.CSP_DEFAULT_ROUTE, csp.CSP_ID_HOST_SIZE, csp_if_lo, csp.CSP_NODE_MAC)
csp.csp_route_start_task(0, 0)

sock = csp.csp_socket(csp.CSP_SO_CONN_LESS)
if csp.csp_bind(sock, PORT) != 0:
    sys.exit("csp_bind failed")

### Send ###
data = bytes("Are you there CSP!!!", encoding="ascii")
data_len = len(data)
packet = csp.csp_buffer_get(data_len);
if not packet:
    raise Exception("Can't allocate packet")
packet = ffi.cast('csp_packet_t *', packet)
packet.data[0:data_len] = data
packet.length = data_len
if 0 != csp.csp_sendto(csp.CSP_PRIO_NORM, NODE_ID, PORT, PORT, csp.CSP_SO_NONE,
                       packet, TIMEOUT):
    csp.csp_buffer_free(packet)
    sys.exit("csp_sendto failed")

### Receive ###
packet_recv = csp.csp_recvfrom(sock, TIMEOUT)
print(bytes(packet_recv.data[0:packet_recv.length]))

#### Send ###
data = bytes("Hey I am here!!!", encoding="ascii")
data_len = len(data)
packet.data[0:data_len] = data
packet.length = data_len
if 0 != csp.csp_sendto(csp.CSP_PRIO_NORM, NODE_ID, PORT, PORT, csp.CSP_SO_NONE,
                       packet, TIMEOUT):
    csp.csp_buffer_free(packet)
    sys.exit("csp_sendto failed")

### Receive ###
packet_recv = csp.csp_recvfrom(sock, TIMEOUT)
print(bytes(packet_recv.data[0:packet_recv.length]))
csp.csp_buffer_free(packet_recv)

packet_recv = csp.csp_recvfrom(sock, TIMEOUT)
if packet_recv:
    sys.exit("It should be a NULL packet")
