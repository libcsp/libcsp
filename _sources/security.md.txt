# Security model and deployment assumptions

libcsp provides networking and management building blocks for embedded systems.
Applications and deployments decide which peers can reach a node, which services
the node handles, and which link or network protections are required. A service
must not be treated as an authentication or authorization boundary unless its
documentation explicitly defines it as one.

(cmp-peek-and-poke)=
## CMP PEEK and POKE

CSP Management Protocol (CMP) PEEK and POKE are remote management and debugging
operations. PEEK reads memory from a CSP node, and POKE writes memory on a CSP
node. This memory access is intentional and is powerful by design.

These operations are intended for deployments in which access to the relevant
CSP management services is already restricted to trusted peers. PEEK and POKE
do not implement authentication, access-control lists, capabilities, or another
authorization boundary. The CMP handler does not decide whether the requesting
peer is permitted to access memory. A peer permitted to issue functional PEEK
or POKE requests is therefore being given memory-access capability by design.

If an untrusted or compromised peer can reach functional CMP PEEK or POKE
operations, it may be able to read sensitive memory, modify memory, crash the
target, or otherwise compromise the node. Applications must not expose these
operations to untrusted peers and rely on the CMP handler to reject unauthorized
memory access.

When a CSP network crosses trust boundaries, the deployment must restrict
access using protections appropriate to its architecture. These may include
network isolation, routing restrictions, hardened gateways or firewalls,
authenticated or encrypted links, or other external controls. libcsp does not
require one universal deployment mechanism.

`csp_service_handler()` handles packets sent to the CMP port and dispatches PEEK
and POKE requests. Consequently, an application that accepts the CMP port and
passes those packets to `csp_service_handler()` exposes the operations to peers
that can reach that service.

The current implementations differ:

- Legacy PEEK and POKE are functional by default.
- PEEK v2 and POKE v2 do not access memory by default. They are functional only
  when the platform or application provides the required implementation.

The intentional semantics above are distinct from an accidental implementation
defect. A report that a reachable, functional PEEK or POKE command performs its
documented memory read or write describes the capability itself. Out-of-bounds
accesses, incorrect length validation, use-after-free defects, parser bugs,
memory corruption, or bypasses of security properties that libcsp claims to
provide are separate issues. The existence of an intentionally powerful command
does not make such defects acceptable. Before reporting PEEK or POKE behavior as
a security defect, verify that the finding goes beyond the documented memory
access capability.
