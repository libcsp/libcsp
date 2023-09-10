---
hide-toc: true
---

```{eval-rst}
.. include:: ../README.md
   :parser: myst_parser.sphinx_
```

```{toctree}
:hidden:

history
```

```{toctree}
:caption: Setup
:hidden:

INSTALL
CI
```

```{toctree}
:caption: CSP API
:hidden:

api/csp_h
api/csp_types_h
api/csp_buffer_h
api/csp_cmp_h
api/csp_sfp_h
api/csp_promisc_h
api/csp_interface_h
api/csp_rtable_h
api/csp_iflist_h
api/csp_error_h
api/csp_debug_h
api/csp_id_h
api/csp_crc32_h
api/csp_hooks_h
api/csp_yaml_h
api/drivers/drivers
```

```{toctree}
:caption: The Cubesat Space Protocol
:hidden:

basic
memory
mtu
outflow
protocolstack
topology
tunnel
hooks
```

```{toctree}
:caption: Development
:hidden:

codestyle
structure
license
```

```{toctree}
:caption: Example Usage
:hidden:

example
```

- [History](history.md)
- [Structure](structure.md)
- [The basics of CSP](basic.md)
- [How CSP uses memory](memory.md)
- [The Protocol Stack](protocolstack.md)
- [Network Topology](topology.md)
- [Maximum Transfer Unit](mtu.md)
- [Client and server example](example.md)
