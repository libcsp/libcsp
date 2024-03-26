CSP Library Header
==================

.. autocmodule:: csp.h

.. contents::
    :depth: 3

Defines
-------

.. autocmacro:: csp.h::CSP_MAX_TIMEOUT
.. autocmacro:: csp.h::CSP_MAX_DELAY

..  autocenumerator:: csp.h::csp_dedup_types
    :members:

Typedefs
--------

.. autoctype:: csp.h::csp_conf_t
    :members:

Interface Functions
-------------------

.. autocfunction:: csp.h::csp_init
.. autocfunction:: csp.h::csp_free_resources
.. autocfunction:: csp.h::csp_get_conf
.. autocfunction:: csp.h::csp_id_copy
.. autocfunction:: csp.h::csp_accept
.. autocfunction:: csp.h::csp_read
.. autocfunction:: csp.h::csp_send
.. autocfunction:: csp.h::csp_send_prio
.. autocfunction:: csp.h::csp_transaction_w_opts
.. autocfunction:: csp.h::csp_transaction
.. autocfunction:: csp.h::csp_transaction_persistent
.. autocfunction:: csp.h::csp_recvfrom
.. autocfunction:: csp.h::csp_sendto
.. autocfunction:: csp.h::csp_sendto_reply
.. autocfunction:: csp.h::csp_connect
.. autocfunction:: csp.h::csp_close
.. autocfunction:: csp.h::csp_socket_close
.. autocfunction:: csp.h::csp_conn_dport
.. autocfunction:: csp.h::csp_conn_sport
.. autocfunction:: csp.h::csp_conn_dst
.. autocfunction:: csp.h::csp_conn_src
.. autocfunction:: csp.h::csp_conn_flags
.. autocfunction:: csp.h::csp_listen
.. autocfunction:: csp.h::csp_bind
.. autocfunction:: csp.h::csp_bind_callback
.. autocfunction:: csp.h::csp_route_work
.. autocfunction:: csp.h::csp_bridge_set_interfaces
.. autocfunction:: csp.h::csp_bridge_work
.. autocfunction:: csp.h::csp_service_handler
.. autocfunction:: csp.h::csp_ping
.. autocfunction:: csp.h::csp_ping_noreply
.. autocfunction:: csp.h::csp_ps
.. autocfunction:: csp.h::csp_get_memfree
.. autocfunction:: csp.h::csp_memfree
.. autocfunction:: csp.h::csp_get_buf_free
.. autocfunction:: csp.h::csp_buf_free
.. autocfunction:: csp.h::csp_reboot
.. autocfunction:: csp.h::csp_shutdown
.. autocfunction:: csp.h::csp_uptime
.. autocfunction:: csp.h::csp_get_uptime
.. autocfunction:: csp.h::csp_rdp_set_opt
.. autocfunction:: csp.h::csp_rdp_get_opt
.. autocfunction:: csp.h::csp_cmp_set_memcpy
.. autocfunction:: csp.h::csp_conn_print_table
.. autocfunction:: csp.h::csp_hex_dump
.. autocfunction:: csp.h::csp_conn_print_table_str
