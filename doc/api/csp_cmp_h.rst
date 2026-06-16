CSP Management Protocol (CMP)
=============================

CMP PEEK and POKE intentionally provide memory access and assume that the
management service is restricted to trusted peers. See :ref:`cmp-peek-and-poke`
for their security model and deployment requirements.

.. autocmodule:: csp_cmp.h

.. contents::
    :depth: 3

Enums
-----

.. autocenum:: csp_cmp.h::csp_cmp_type_t
.. autocenum:: csp_cmp.h::csp_cmp_code_t

Interface Functions
-------------------

.. autocfunction:: csp_cmp.h::csp_cmp
.. autocfunction:: csp_cmp.h::csp_cmp_peek
.. autocfunction:: csp_cmp.h::csp_cmp_poke

Time Sync
---------

The CMP time-sync service uses broadcast SYNC/FUP messages on ``CSP_CMP``. The
wire format is fixed in CSP, while local timestamp capture and time-discipline
updates are provided by the user through hooks.

Receivers store SYNC by ``(master, id, tv_sec)`` and only accept a FUP from the
same master with the same id and origin second.
