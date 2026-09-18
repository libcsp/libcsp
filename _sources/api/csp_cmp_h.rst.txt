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
