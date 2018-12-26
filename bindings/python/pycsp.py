# Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
# Copyright (C) 2012 GomSpace ApS (http://www.gomspace.com)
# Copyright (C) 2012 AAUSAT3 Project (http://aausat3.space.aau.dk) 
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

# Python bindings for Cubesat Space Protocol

import ctypes
import ctypes.util
import time
import sys
import os

# CSP Configuration
CSP_MAX_BIND_PORT   = 31

# NULL pointer Exception
class NullPointerException(Exception):
	pass

# Generic CSP Exception
class CspException(Exception):
	def __init__(self, value):
		self.value = value
	def __str__(self):
		return str(self.value)

# Null pointer error function
def null_pointer_err(result, func, args):
	if bool(result) != True:
		raise NullPointerException
	else:
		return result
		
# Generic error function
def csp_err(result, func, args):
	if result != 1:
		raise CspException(os.strerror(ctypes.get_errno()))
	else:
		return result

def csp_err_negative(result, func, args):
	if result != 0:
		raise CspException(os.strerror(ctypes.get_errno()))
	else:
		return result

# Reserved Ports (Services)
CSP_PING			= 1
CSP_PS			 	= 2
CSP_MEMFREE		 	= 3
CSP_REBOOT		  	= 4
CSP_BUF_FREE		= 5
CSP_UPTIME			= 6
CSP_ANY			 	= (CSP_MAX_BIND_PORT + 1)
CSP_PROMISC		 	= (CSP_MAX_BIND_PORT + 2)

# Priorities
CSP_PRIO_CRITICAL   = 0
CSP_PRIO_HIGH	   	= 1
CSP_PRIO_NORM	   	= 2
CSP_PRIO_LOW		= 3

# Define used to specify MAC_ADDR = NODE_ID
CSP_NODE_MAC		= 0xFF

# Size of bit-fields in CSP header
CSP_ID_PRIO_SIZE	= 2
CSP_ID_HOST_SIZE	= 5
CSP_ID_PORT_SIZE	= 6
CSP_ID_FLAGS_SIZE   = 8

if CSP_ID_PRIO_SIZE + 2 * CSP_ID_HOST_SIZE + 2 * CSP_ID_PORT_SIZE + CSP_ID_FLAGS_SIZE != 32:
	print "CSP header lenght must be 32 bits"

# Highest number to be entered in field
CSP_ID_PRIO_MAX		= (1 << CSP_ID_PRIO_SIZE) - 1
CSP_ID_HOST_MAX		= (1 << CSP_ID_HOST_SIZE) - 1
CSP_ID_PORT_MAX		= (1 << CSP_ID_PORT_SIZE) - 1
CSP_ID_FLAGS_MAX	= (1 << CSP_ID_FLAGS_SIZE) - 1

# Identifier field masks
CSP_ID_PRIO_MASK	= CSP_ID_PRIO_MAX	<< (CSP_ID_FLAGS_SIZE + 2 * CSP_ID_PORT_SIZE + 2 * CSP_ID_HOST_SIZE)
CSP_ID_SRC_MASK		= CSP_ID_HOST_MAX	<< (CSP_ID_FLAGS_SIZE + 2 * CSP_ID_PORT_SIZE + 1 * CSP_ID_HOST_SIZE)
CSP_ID_DST_MASK		= CSP_ID_HOST_MAX	<< (CSP_ID_FLAGS_SIZE + 2 * CSP_ID_PORT_SIZE)
CSP_ID_DPORT_MASK   = CSP_ID_PORT_MAX	<< (CSP_ID_FLAGS_SIZE + 1 * CSP_ID_PORT_SIZE)
CSP_ID_SPORT_MASK   = CSP_ID_PORT_MAX	<< (CSP_ID_FLAGS_SIZE)
CSP_ID_FLAGS_MASK   = CSP_ID_FLAGS_MAX	<< (0)

CSP_ID_CONN_MASK	= CSP_ID_SRC_MASK | CSP_ID_DST_MASK | CSP_ID_DPORT_MASK | CSP_ID_SPORT_MASK

# Broadcast address
CSP_BROADCAST_ADDR  = CSP_ID_HOST_MAX

# Default routing address
CSP_DEFAULT_ROUTE   = CSP_ID_HOST_MAX + 1

# CSP Flags
CSP_FRES1			= 0x80 # Reserved for future use
CSP_FRES2			= 0x40 # Reserved for future use
CSP_FRES3			= 0x20 # Reserved for future use
CSP_FRES4			= 0x10 # Reserved for future use
CSP_FHMAC			= 0x08 # Use HMAC verification/generation
CSP_FXTEA			= 0x04 # Use XTEA encryption/decryption
CSP_FRDP			= 0x02 # Use RDP protocol
CSP_FCRC			= 0x01 # Use CRC32 checksum (Not implemented)

# CSP Socket options
CSP_SO_RDPREQ		= 0x0001
CSP_SO_HMACREQ		= 0x0002
CSP_SO_XTEAREQ		= 0x0004

# CSP Connect options
CSP_O_RDP			= CSP_SO_RDPREQ
CSP_O_HMAC			= CSP_SO_HMACREQ
CSP_O_XTEA			= CSP_SO_XTEAREQ

# CSP Packet Identifier
if sys.byteorder == 'little':
	class csp_id_fields_t (ctypes.Structure):
		_pack_ = 1
		_fields_ = [("flags", ctypes.c_uint, 8),
					("sport", ctypes.c_uint, 6),
					("dport", ctypes.c_uint, 6),
					("dst", ctypes.c_uint, 5),
					("src", ctypes.c_uint, 5),
					("pri", ctypes.c_uint, 2)]
elif sys.byteorder == 'big':
	class csp_id_fields_t (ctypes.Structure):
		_pack_ = 1
	_fields_ = [("pri", ctypes.c_uint, 2),
					("src", ctypes.c_uint, 5),
					("dst", ctypes.c_uint, 5),
					("dport", ctypes.c_uint, 6),
					("sport", ctypes.c_uint, 6),
					("flags", ctypes.c_uint, 8)]
else:
	raise Exception("sys.byteorder was neither little or big")

class csp_id_t (ctypes.Union):
	_pack_ = 1
	_anonymous_ = ("id",)
	_fields_ = [("ext", ctypes.c_uint32),
				("id", csp_id_fields_t)]

#typedef struct {
#	uint8_t padding[44];
#	uint16_t length;
#	csp_id_t id;
#	uint8_t data[0];
#} csp_packet_t;

class csp_packet_t (ctypes.Structure):
	# Flexible length arrays are not supported in ctypes
	_pack_ = 1
	_fields_ = [("padding", ctypes.c_uint8 * 46),
				("length", ctypes.c_uint16),
				("id", csp_id_t),
				("data", ctypes.c_uint8 * 256)]
				
CSP_BUFFER_PACKET_OVERHEAD  = 46+2+4

# typedef struct csp_socket_s csp_socket_t;
class csp_socket_t (ctypes.Structure):
	pass
	
# typedef struct csp_conn_s csp_conn_t;
class csp_conn_t (ctypes.Structure):
	pass

# typedef struct csp_l4data_s csp_l4data_t;
class csp_l4data_t (ctypes.Structure):
	pass

# Load library
libcsp = ctypes.CDLL("libpycsp.so", use_errno=True)

my_address = libcsp.my_address

# Function prototypes

# csp_io.c
csp_init = libcsp.csp_init
csp_init.argtypes = [ctypes.c_uint8]
csp_init.restype = None

csp_socket = libcsp.csp_socket
csp_socket.argtypes = [ctypes.c_uint32]
csp_socket.restype = ctypes.POINTER(csp_socket_t)
csp_socket.errcheck = null_pointer_err

csp_accept = libcsp.csp_accept
csp_accept.argtypes = [ctypes.POINTER(csp_socket_t), ctypes.c_uint]
csp_accept.restype = ctypes.POINTER(csp_conn_t)

csp_read = libcsp.csp_read
csp_read.argtypes = [ctypes.POINTER(csp_conn_t), ctypes.c_uint]
csp_read.restype = ctypes.POINTER(csp_packet_t)
csp_read.errcheck = null_pointer_err

csp_send = libcsp.csp_send
csp_send.argtypes = [ctypes.POINTER(csp_conn_t), ctypes.POINTER(csp_packet_t), ctypes.c_uint]
csp_send.restype = ctypes.c_int
csp_send.errcheck = csp_err

csp_transaction = libcsp.csp_transaction
csp_transaction.argtypes = [ctypes.c_uint8, ctypes.c_uint8, ctypes.c_uint8, ctypes.c_uint, ctypes.c_void_p, ctypes.c_int, ctypes.c_void_p, ctypes.c_int]
csp_transaction.restype = ctypes.c_int

csp_transaction_persistent = libcsp.csp_transaction_persistent
csp_transaction_persistent.argtypes = [ctypes.POINTER(csp_conn_t), ctypes.c_uint, ctypes.c_void_p, ctypes.c_int, ctypes.c_void_p, ctypes.c_int]
csp_transaction_persistent.restype = ctypes.c_int

# csp_conn.c
csp_connect = libcsp.csp_connect
csp_connect.argtypes = [ctypes.c_uint8, ctypes.c_uint8, ctypes.c_uint8, ctypes.c_uint, ctypes.c_uint32]
csp_connect.restype = ctypes.POINTER(csp_conn_t)
csp_connect.errcheck = null_pointer_err

csp_close = libcsp.csp_close
csp_close.argtypes = [ctypes.POINTER(csp_conn_t)]
csp_close.restype = None

csp_conn_dport = libcsp.csp_conn_dport
csp_conn_dport.argtypes = [ctypes.POINTER(csp_conn_t)]
csp_conn_dport.restype = ctypes.c_int

csp_conn_sport = libcsp.csp_conn_sport
csp_conn_sport.argtypes = [ctypes.POINTER(csp_conn_t)]
csp_conn_sport.restype = ctypes.c_int

csp_conn_dst = libcsp.csp_conn_dst
csp_conn_dst.argtypes = [ctypes.POINTER(csp_conn_t)]
csp_conn_dst.restype = ctypes.c_int

csp_conn_src = libcsp.csp_conn_src
csp_conn_src.argtypes = [ctypes.POINTER(csp_conn_t)]
csp_conn_src.restype = ctypes.c_int

# Define csp_conn_print if libcsp was compiled with CSP_DEBUG
try:
	csp_conn_print_table = libcsp.csp_conn_print_table
	csp_conn_print_table.argtypes = None
	csp_conn_print_table.restype = None
except:
	pass

# csp_port.c
csp_listen = libcsp.csp_listen
csp_listen.argtypes = [ctypes.POINTER(csp_socket_t), ctypes.c_size_t]
csp_listen.restype = ctypes.c_int

csp_bind = libcsp.csp_bind
csp_bind.argtypes = [ctypes.POINTER(csp_socket_t), ctypes.c_uint8]
csp_bind.restype = ctypes.c_int

# csp_route.c
#typedef int (*nexthop_t)(csp_id_t idout, csp_packet_t * packet, uint32_t timeout);

csp_route_set = libcsp.csp_route_set
# TODO, add argtypes (i.e. figure out how to define function pointer)
csp_route_set.restype = None

csp_route_start_task = libcsp.csp_route_start_task
csp_route_start_task.argtypes = [ctypes.c_uint, ctypes.c_uint]
csp_route_start_task.restype = None

csp_promisc_enable = libcsp.csp_promisc_enable
csp_promisc_enable.argtypes = [ctypes.c_uint]
csp_promisc_enable.restype = ctypes.c_int

csp_promisc_read = libcsp.csp_promisc_read
csp_promisc_read.argtypes = [ctypes.c_uint]
csp_promisc_read.restype = ctypes.POINTER(csp_packet_t)
csp_promisc_read.errcheck = null_pointer_err

# csp_services.c
csp_service_handler = libcsp.csp_service_handler
csp_service_handler.argtypes = [ctypes.POINTER(csp_conn_t), ctypes.POINTER(csp_packet_t)]
csp_service_handler.restype = None

csp_ping = libcsp.csp_ping
csp_ping.argtypes = [ctypes.c_uint8, ctypes.c_uint]
csp_ping.restype = ctypes.c_int 

csp_ping_noreply = libcsp.csp_ping_noreply
csp_ping_noreply.argtypes = [ctypes.c_uint8]
csp_ping_noreply.restype = None

csp_ps = libcsp.csp_ps
csp_ps.argtypes = [ctypes.c_uint8, ctypes.c_uint]
csp_ps.restype = None

csp_memfree = libcsp.csp_memfree
csp_memfree.argtypes = [ctypes.c_uint8, ctypes.c_uint]
csp_memfree.restype = None

csp_buf_free = libcsp.csp_buf_free
csp_buf_free.argtypes = [ctypes.c_uint8, ctypes.c_uint]
csp_buf_free.restype = None

csp_reboot = libcsp.csp_reboot
csp_reboot.argtypes = [ctypes.c_uint8, ctypes.c_uint]
csp_reboot.restype = None

csp_buffer_init = libcsp.csp_buffer_init
csp_buffer_init.argtypes = [ctypes.c_int, ctypes.c_int]
csp_buffer_init.restype = ctypes.c_int

csp_buffer_get = libcsp.csp_buffer_get
csp_buffer_get.argtypes = [ctypes.c_size_t]
csp_buffer_get.restype = ctypes.c_void_p
csp_buffer_get.errcheck = null_pointer_err

csp_buffer_get_isr = libcsp.csp_buffer_get
csp_buffer_get_isr.argtypes = [ctypes.c_size_t]
csp_buffer_get_isr.restype = ctypes.c_void_p
csp_buffer_get_isr.errcheck = null_pointer_err

csp_buffer_free = libcsp.csp_buffer_free
csp_buffer_free.argtypes = [ctypes.c_void_p]
csp_buffer_free.restype = None

csp_buffer_remaining = libcsp.csp_buffer_remaining
csp_buffer_remaining.argtypes = None
csp_buffer_remaining.restype = ctypes.c_int

# Define csp_buffer_print if libcsp was compiled with CSP_DEBUG
DEBUG_FUNC = ctypes.CFUNCTYPE(None, ctypes.c_int, ctypes.c_char_p)
try:
	csp_debug_hook_set = libcsp.csp_debug_hook_set
	csp_debug_hook_set.argtypes = [DEBUG_FUNC]
	csp_debug_hook_set.restype = None
except:
	pass

# Debug hook
CSP_INFO = 0
CSP_ERROR = 1
CSP_WARN = 2
CSP_BUFFER = 3
CSP_PACKET = 4
CSP_PROTOCOL = 5
CSP_LOCK = 6

# CAN Interface - this should be moved to it's own bindings
class can_socketcan_conf (ctypes.Structure):
	_fields_ = [("ifc", ctypes.c_char_p)]

try:
    csp_if_can = libcsp.csp_if_can
    csp_can_init = libcsp.csp_can_init
    csp_can_init.argtypes = [ctypes.c_uint8, ctypes.c_void_p, ctypes.c_uint]
    csp_can_init.restype = ctypes.c_int
    csp_can_init.errcheck = csp_err_negative

    csp_can_tx = libcsp.csp_can_tx
    csp_can_tx.argtypes = [csp_id_t, ctypes.POINTER(csp_packet_t), ctypes.c_uint]
    csp_can_tx.restype = ctypes.c_int
except:
    pass
