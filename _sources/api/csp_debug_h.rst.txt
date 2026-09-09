CSP Debug
=========

.. autocmodule:: csp_debug.h

Variables
---------

.. autocdata:: csp_debug.h::csp_dbg_buffer_out

.. autocdata:: csp_debug.h::csp_dbg_conn_out

.. autocdata:: csp_debug.h::csp_dbg_conn_ovf

.. autocdata:: csp_debug.h::csp_dbg_conn_noroute

.. autocdata:: csp_debug.h::csp_dbg_inval_reply

.. autocdata:: csp_debug.h::csp_dbg_errno

.. autocdata:: csp_debug.h::csp_dbg_can_errno

.. autocdata:: csp_debug.h::csp_dbg_eth_errno

.. autocdata:: csp_debug.h::csp_dbg_rdp_print

.. autocdata:: csp_debug.h::csp_dbg_packet_print

Defines
-------

General error codes
~~~~~~~~~~~~~~~~~~~

.. autocmacro:: csp_debug.h::CSP_DBG_ERR_CORRUPT_BUFFER
.. autocmacro:: csp_debug.h::CSP_DBG_ERR_MTU_EXCEEDED
.. autocmacro:: csp_debug.h::CSP_DBG_ERR_ALREADY_FREE
.. autocmacro:: csp_debug.h::CSP_DBG_ERR_REFCOUNT
.. autocmacro:: csp_debug.h::CSP_DBG_ERR_INVALID_RTABLE_ENTRY
.. autocmacro:: csp_debug.h::CSP_DBG_ERR_UNSUPPORTED
.. autocmacro:: csp_debug.h::CSP_DBG_ERR_INVALID_BIND_PORT
.. autocmacro:: csp_debug.h::CSP_DBG_ERR_PORT_ALREADY_IN_USE
.. autocmacro:: csp_debug.h::CSP_DBG_ERR_ALREADY_CLOSED
.. autocmacro:: csp_debug.h::CSP_DBG_ERR_INVALID_POINTER
.. autocmacro:: csp_debug.h::CSP_DBG_ERR_CLOCK_SET_FAIL

CAN-specific error codes
~~~~~~~~~~~~~~~~~~~~~~~~

.. autocmacro:: csp_debug.h::CSP_DBG_CAN_ERR_FRAME_LOST
.. autocmacro:: csp_debug.h::CSP_DBG_CAN_ERR_RX_OVF
.. autocmacro:: csp_debug.h::CSP_DBG_CAN_ERR_RX_OUT
.. autocmacro:: csp_debug.h::CSP_DBG_CAN_ERR_SHORT_BEGIN
.. autocmacro:: csp_debug.h::CSP_DBG_CAN_ERR_INCOMPLETE
.. autocmacro:: csp_debug.h::CSP_DBG_CAN_ERR_UNKNOWN

ETH-specific error codes
~~~~~~~~~~~~~~~~~~~~~~~~

.. autocmacro:: csp_debug.h::CSP_DBG_ETH_ERR_FRAME_LOST
.. autocmacro:: csp_debug.h::CSP_DBG_ETH_ERR_RX_OVF
.. autocmacro:: csp_debug.h::CSP_DBG_ETH_ERR_RX_OUT
.. autocmacro:: csp_debug.h::CSP_DBG_ETH_ERR_SHORT_BEGIN
.. autocmacro:: csp_debug.h::CSP_DBG_ETH_ERR_INCOMPLETE
.. autocmacro:: csp_debug.h::CSP_DBG_ETH_ERR_UNKNOWN

Print macros
~~~~~~~~~~~~

.. autocmacro:: csp_debug.h::csp_print
.. autocmacro:: csp_debug.h::csp_rdp_error
.. autocmacro:: csp_debug.h::csp_rdp_protocol
.. autocmacro:: csp_debug.h::csp_print_packet

Functions
---------

.. autocfunction:: csp_debug.h::csp_print_func
