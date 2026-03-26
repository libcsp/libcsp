# CSP Routing Implementation Analysis

This document provides a comprehensive analysis of the Cubesat Space Protocol (CSP) routing
implementation, focusing on how packets are routed through the network.

## Table of Contents

1. [Addressing and Routing Relationship](#1-addressing-and-routing-relationship)
2. [Multiple Physical Interfaces with Same Address](#2-multiple-physical-interfaces-with-same-address)
3. [VIA Address Mechanism](#3-via-address-mechanism)
4. [Practical Routing Scenarios](#4-practical-routing-scenarios)
5. [Interface Selection Control](#5-interface-selection-control)
6. [Visual Documentation](#6-visual-documentation)

---

## 1. Addressing and Routing Relationship

### CSP Address Structure

CSP supports two protocol versions with different address sizes:

| Version | Address Bits | Max Addresses | Port Bits |
|---------|--------------|---------------|-----------|
| CSP v1  | 5 bits       | 0-31          | 6 bits    |
| CSP v2  | 14 bits      | 0-16383       | 6 bits    |

Each CSP packet header contains:
- **Source address** (`id.src`): Originating node
- **Destination address** (`id.dst`): Target node
- **Source/Destination ports**: Application-level endpoints
- **Priority and Flags**: QoS and feature flags

### How Routing Table Lookup Works

The routing decision process in `csp_send_direct()` (src/csp_io.c:87-217) follows a
three-tier priority system:

```
┌─────────────────────────────────────────────────────────────────┐
│                  ROUTING DECISION PRIORITY                       │
├─────────────────────────────────────────────────────────────────┤
│ 1. LOOPBACK      → If dst == csp_if_lo.addr, use loopback       │
│ 2. LOCAL SUBNET  → Check interfaces where dst is within subnet  │
│ 3. ROUTING TABLE → Use csp_rtable_find_route() for best match   │
│ 4. DEFAULT IFACE → Fall back to interfaces marked is_default=1  │
└─────────────────────────────────────────────────────────────────┘
```

### Route Lookup: `csp_rtable_find_route()` (src/csp_rtable_cidr.c:44-74)

The routing table uses **longest prefix matching** (CIDR-style):

```c
csp_route_t * csp_rtable_find_route(uint16_t addr) {
    int best_result = -1;
    uint16_t best_result_mask = 0;

    for (int i = 0; i < rtable_inptr; i++) {
        uint16_t hostbits = (1 << (csp_id_get_host_bits() - rtable[i].netmask)) - 1;
        uint16_t netbits = ~hostbits;

        uint16_t net_a = rtable[i].address & netbits;
        uint16_t net_b = addr & netbits;

        if (net_a == net_b) {
            if (rtable[i].netmask >= best_result_mask) {  // Longest match wins
                best_result = i;
                best_result_mask = rtable[i].netmask;
            }
        }
    }
    return (best_result > -1) ? &rtable[best_result] : NULL;
}
```

**Key points:**
- Routes with **larger netmask values** (more specific) take precedence
- A route `10/5` (address 10, netmask 5) matches all addresses from 0-31 that share the
  same upper bits as 10
- The default route `0/0` matches everything (lowest priority)

### Netmask Calculation

For CSP v2 with 14 host bits, a netmask of 8 means:
- **Network bits**: Upper 8 bits identify the subnet
- **Host bits**: Lower 6 bits (14-8=6) identify hosts within subnet
- **Available hosts**: 2^6 - 1 = 63 (minus 1 for broadcast)

```
Example: Address 128 with netmask 8
Binary:  00000010 000000 (128 = 0b10000000)
         ^^^^^^^^ ^^^^^^
         Network  Host
         (8 bits) (6 bits)

Subnet range: 128-191 (addresses where upper 8 bits = 0b10000000)
```

---

## 2. Multiple Physical Interfaces with Same Address

### Can a Node Use the Same CSP Address Across Multiple Interfaces?

**Yes**, CSP explicitly supports this configuration. From `doc/topology.md`:

> "In this case, the two (or more) physical interfaces in the sub-systems shall be
> configured to have the same CSP address and related properties, by which the CSP
> router knows that packets for that particular logical CSP subnet shall be transmitted
> on both physical interfaces."

This is commonly used for **redundant physical networks** (e.g., dual CAN buses).

### Requirements and Limitations

1. **Same subnet configuration**: Both interfaces must have identical `addr` and `netmask`
2. **Packet duplication**: When sending, packets are cloned and sent on ALL matching interfaces
3. **Deduplication required**: Enable `csp_conf.dedup` to prevent processing duplicate packets

### Split Horizon Mechanism

Split horizon prevents routing loops by never sending a packet back to:
1. The **same interface** it arrived on
2. Any interface in the **same subnet** as the source interface

**Implementation in `csp_send_direct()`** (src/csp_io.c:110-119):

```c
/* Do not send back to same interface (split horizon) */
if (iface == routed_from) {
    continue;
}

/* Do not send to interface with similar subnet (split horizon) */
if (csp_iflist_is_within_subnet(iface->addr, routed_from)) {
    continue;
}
```

**Subnet comparison** (src/csp_iflist.c:15-31):

```c
int csp_iflist_is_within_subnet(uint16_t addr, csp_iface_t * ifc) {
    if (ifc == NULL) return 0;

    uint16_t netmask = ((1 << ifc->netmask) - 1) << (csp_id_get_host_bits() - ifc->netmask);
    uint16_t network_a = ifc->addr & netmask;
    uint16_t network_b = addr & netmask;

    return (network_a == network_b) ? 1 : 0;
}
```

---

## 3. VIA Address Mechanism

### What is the VIA Address?

The `via` field in `csp_route_t` specifies an **intermediate node address** (next-hop gateway)
for routing packets that cannot be delivered directly.

```c
typedef struct csp_route_s {
    uint16_t address;   // Destination address/network
    uint16_t netmask;   // Number of significant bits
    uint16_t via;       // Next-hop address (or CSP_NO_VIA_ADDRESS)
    csp_iface_t * iface; // Outgoing interface
} csp_route_t;
```

### CSP_NO_VIA_ADDRESS (0xFFFF)

When `via == CSP_NO_VIA_ADDRESS`:
- The packet is sent **directly** to the destination address in the CSP header
- The physical layer uses `packet->id.dst` for link-layer addressing

When `via` is a valid address:
- The packet's CSP header **retains the original destination**
- The **physical layer** uses the `via` address for link-layer delivery
- The via node will forward the packet to its final destination

### VIA Address Application (src/csp_io.c:271)

```c
if ((*iface->nexthop)(iface, via, packet, from_me) != CSP_ERR_NONE)
    goto tx_err;
```

The `via` parameter is passed to each interface's `nexthop` function.

### Interface-Specific VIA Handling

**I2C Interface** (src/interfaces/csp_if_i2c.c:18-22):
```c
/* Use cfpid to transfer the physical destination address */
packet->cfpid = (via != CSP_NO_VIA_ADDRESS) ? via : packet->id.dst;
packet->cfpid = packet->cfpid & 0x7F;  // 7-bit I2C address
```

**CAN Interface** (src/interfaces/csp_if_can.c:178-179):
```c
/* Figure out destination node based on routing entry */
const uint8_t dest = (via != CSP_NO_VIA_ADDRESS) ? via : packet->id.dst;
```

**KISS Interface** (src/interfaces/csp_if_kiss.c:23):
- Ignores `via` - KISS is point-to-point and doesn't need link-layer addressing

### Relationship: VIA vs Link-Layer Addressing

```
┌─────────────────────────────────────────────────────────────────────┐
│                    CSP Packet on Wire                                │
├─────────────────────────────────────────────────────────────────────┤
│  Link Layer Header (CAN ID / I2C Addr)  │  CSP Header  │  Data     │
│         ↑                               │      ↑       │           │
│    Uses VIA address                     │  Original    │           │
│    (or dst if no VIA)                   │  src/dst     │           │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 4. Practical Routing Scenarios

### Scenario A: OBC → Radio → Ground Station

```
┌─────────┐    CAN     ┌─────────┐    RF     ┌─────────┐
│   OBC   │───────────│  Radio  │ ~ ~ ~ ~ ~ │   TNC   │
│ Addr: 1 │           │ Addr: 2 │           │ Addr:192│
└─────────┘           └─────────┘           └─────────┘
                                                  │
                                                  │ KISS
                                                  ↓
                                            ┌─────────┐
                                            │   PC    │
                                            │ Addr:193│
                                            └─────────┘

Space Segment: 0/8 subnet (addresses 0-63)
Ground Segment: 192/8 subnet (addresses 192-255)
```

**Routing Table Configurations:**

**OBC (Address 1):**
```
csp_rtable_load("0/8 CAN, 192/8 CAN 2")
```
- `0/8 CAN`: Local subnet traffic goes directly on CAN
- `192/8 CAN 2`: Ground segment traffic → CAN interface **via node 2** (Radio)

**Radio (Address 2):**
```
csp_rtable_load("0/8 CAN, 192/8 RF")
```
- `0/8 CAN`: Space segment on CAN bus
- `192/8 RF`: Ground segment on RF interface (direct delivery)

**TNC (Address 192):**
```
csp_rtable_load("192/8 KISS, 0/8 RF")
```
- `192/8 KISS`: Ground segment on KISS (to PC)
- `0/8 RF`: Space segment via RF

**Ground PC (Address 193):**
```
csp_rtable_load("192/8 KISS, 0/8 KISS 192")
```
- `192/8 KISS`: Local subnet direct
- `0/8 KISS 192`: Space segment **via TNC (192)**

**Step-by-Step Packet Flow: OBC (1) sends to PC (193)**

1. **OBC** creates packet: `src=1, dst=193`
2. Route lookup finds `192/8 CAN 2` → Send on CAN with `via=2`
3. CAN frame addressed to node 2, CSP header shows `dst=193`
4. **Radio** receives packet, checks `dst=193` (not to me)
5. Route lookup finds `192/8 RF` → Send on RF with `via=CSP_NO_VIA_ADDRESS`
6. **TNC** receives packet, checks `dst=193` (not to me)
7. Route lookup finds `192/8 KISS` → Forward on KISS
8. **PC** receives packet, `dst=193` matches local address → deliver to application

### Scenario B: Multi-Hop Routing

```
┌────────┐   CAN    ┌────────┐  I2C   ┌────────┐  CAN   ┌────────┐
│ Node A │─────────│ Node B │───────│ Node C │───────│ Node D │
│ Addr:1 │         │ Addr:2 │       │ Addr:3 │       │ Addr:4 │
└────────┘         └────────┘       └────────┘       └────────┘
   │                                                      │
   └──────────────── Goal: A sends to D ──────────────────┘
```

**Routing Tables:**

| Node | Address | Routing Table |
|------|---------|---------------|
| A    | 1       | `4 CAN 2` (reach 4 via 2 on CAN) |
| B    | 2       | `4 I2C 3` (reach 4 via 3 on I2C) |
| C    | 3       | `4 CAN` (reach 4 directly on CAN) |
| D    | 4       | `1 CAN 3` (reach 1 via 3 on CAN) |

**Packet Flow A→D:**

```
A                    B                    C                    D
│                    │                    │                    │
│─── CAN (via=2) ───→│                    │                    │
│  [src=1,dst=4]     │                    │                    │
│                    │─── I2C (via=3) ───→│                    │
│                    │   [src=1,dst=4]    │                    │
│                    │                    │─── CAN (direct) ──→│
│                    │                    │   [src=1,dst=4]    │
│                    │                    │                    │✓
```

### Source Address Assignment (`from_me`)

When a packet originates from the local node (`routed_from == NULL`), the `from_me`
flag is set to 1. This triggers automatic source address assignment:

```c
/* Apply outgoing interface address to packet */
if ((from_me) && (idout->src == 0)) {
    idout->src = iface->addr;
}
```

This allows applications to use `src=0` when sending, and CSP automatically fills in
the appropriate interface address.

---

## 5. Interface Selection Control

### Can Senders Explicitly Specify an Interface?

**No**, CSP does not provide a direct API to specify the outgoing interface. Interface
selection is determined by:

1. Destination address matching interface subnets
2. Routing table entries
3. Default interface flag

### Default Route Mechanism (`0/0`)

A route with `address=0` and `netmask=0` matches **all destinations**:

```
csp_rtable_load("0/0 CAN")  // All traffic defaults to CAN interface
```

Since longest-prefix matching is used, more specific routes override the default.

### Alternative: Default Interface Flag

Instead of explicit routing tables, interfaces can be marked as default:

```c
iface->is_default = 1;
```

If no subnet match or routing table entry is found, CSP iterates through all default
interfaces (src/csp_io.c:187-213).

### Routing Through a Specific Node

To route traffic through a specific node (e.g., radio module), you **must** use routing
table entries with a `via` address:

```c
// Route all ground segment traffic through radio at address 2
csp_rtable_set(192, 8, &can_iface, 2);
```

There is no implicit "route through this node" mechanism - explicit configuration required.

### Subnet Routing vs. Routing Table Forwarding

| Feature | Subnet-Based | Routing Table |
|---------|--------------|---------------|
| Configuration | Interface `addr` and `netmask` | `csp_rtable_set()` or `csp_rtable_load()` |
| VIA support | No (direct delivery only) | Yes |
| Use case | Direct neighbors | Multi-hop, gateways |
| Priority | Higher (checked first) | Lower (fallback) |

---

## 6. Visual Documentation

### Packet Flow Through `csp_route_work()`

```
                        ┌─────────────────────────┐
                        │    csp_qfifo_read()     │
                        │   (Get next packet)     │
                        └───────────┬─────────────┘
                                    │
                                    ▼
                        ┌─────────────────────────┐
                        │  csp_input_hook()       │
                        │  (Debug/logging)        │
                        └───────────┬─────────────┘
                                    │
                                    ▼
                        ┌─────────────────────────┐
                        │  is_to_me check:        │
                        │  - dst matches any iface│
                        │  - dst is broadcast     │
                        └───────────┬─────────────┘
                                    │
                    ┌───────────────┴───────────────┐
                    │ is_to_me?                     │
              ┌─────┴─────┐                   ┌─────┴─────┐
              │    NO     │                   │    YES    │
              └─────┬─────┘                   └─────┬─────┘
                    │                               │
                    ▼                               ▼
        ┌───────────────────────┐     ┌───────────────────────┐
        │  csp_send_direct()    │     │ Check callbacks/      │
        │  (Forward packet)     │     │ sockets for dport     │
        └───────────────────────┘     └───────────┬───────────┘
                                                  │
                                    ┌─────────────┴─────────────┐
                                    │                           │
                              ┌─────┴─────┐               ┌─────┴─────┐
                              │ Callback  │               │  Socket   │
                              │  found    │               │  found    │
                              └─────┬─────┘               └─────┬─────┘
                                    │                           │
                                    ▼                           ▼
                              ┌───────────┐             ┌───────────────┐
                              │callback() │             │csp_conn_new() │
                              └───────────┘             │or existing    │
                                                        └───────────────┘
```

### Interface Selection in `csp_send_direct()`

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        csp_send_direct() Flow                                │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  1. LOOPBACK CHECK                                                           │
│     if (dst == csp_if_lo.addr) → send to loopback, DONE                     │
│                                                                              │
│  2. SUBNET MATCHING (iterate all interfaces)                                 │
│     ┌─────────────────────────────────────────────────────────┐             │
│     │ for each iface where csp_iflist_is_within_subnet(dst):  │             │
│     │   - Skip if iface == routed_from (split horizon)        │             │
│     │   - Skip if iface in same subnet as routed_from         │             │
│     │   - Set src = iface->addr if from_me && src==0          │             │
│     │   - Clone packet and send via iface (via=CSP_NO_VIA)    │             │
│     └─────────────────────────────────────────────────────────┘             │
│     if (local_found) → free original packet, DONE                           │
│                                                                              │
│  3. ROUTING TABLE (if CSP_USE_RTABLE)                                        │
│     ┌─────────────────────────────────────────────────────────┐             │
│     │ route = csp_rtable_find_route(dst) // longest match     │             │
│     │ for each matching route:                                 │             │
│     │   - Skip if route->iface == routed_from                 │             │
│     │   - Skip if route->iface in same subnet as routed_from  │             │
│     │   - Set src = route->iface->addr if from_me && src==0   │             │
│     │   - Clone packet and send via route->iface, route->via  │             │
│     └─────────────────────────────────────────────────────────┘             │
│     if (route_found) → free original packet, DONE                           │
│                                                                              │
│  4. DEFAULT INTERFACES (fallback)                                            │
│     ┌─────────────────────────────────────────────────────────┐             │
│     │ for each iface where is_default == 1:                   │             │
│     │   - Skip if iface == routed_from                        │             │
│     │   - Skip if iface in same subnet as routed_from         │             │
│     │   - Set src = iface->addr if from_me && src==0          │             │
│     │   - Clone packet and send via iface (via=CSP_NO_VIA)    │             │
│     └─────────────────────────────────────────────────────────┘             │
│                                                                              │
│  5. FREE original packet (always)                                            │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Network Topology Example with Routing Tables

Based on `doc/topology.md`:

```
  ┌───────┬───────┬───────┬───────┬───────┐
  │       │       │       │       │       │     bus: I2C/CAN (128/8 subnet)
+---+   +---+   +---+   +---+   +---+     │
│OBC│   │COM│   │EPS│   │PL1│   │PL2│     │     Space segment nodes: 128-191
│128│   │129│   │130│   │131│   │132│     │
+---+   +---+   +---+   +---+   +---+     │
          │                               │
          │ Radio Link                    │
          ▼                               │
        +---+                             │     RF link (192/8 subnet)
        │TNC│─────────────────────────────┘
        │192│
        +---+
          │
          │ KISS
          ▼
        +---+
        │ PC│                                   Ground segment: 192-255
        │193│
        +---+
          │
          │ ZMQ
    ┌─────┴─────┐
  +---+       +---+
  │PC2│       │PC3│                             ZMQ subnet (96/9)
  │ 96│       │ 97│
  +---+       +---+
```

**Example Routing Configuration:**

OBC (128):
```
csp_rtable_load("128/8 CAN, 192/8 CAN 129, 96/9 CAN 129")
```

COM/Radio (129):
```
csp_rtable_load("128/8 CAN, 192/8 RF, 96/9 RF")
```

TNC (192):
```
csp_rtable_load("192/8 KISS, 128/8 RF, 96/9 KISS 193")
```

Ground PC (193):
```
csp_rtable_load("192/8 KISS, 128/8 KISS 192, 96/9 ZMQ")
```

### Relationship: CSP Address, VIA Address, and Physical Interface

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                                                                              │
│    Application Layer                                                         │
│    ┌────────────────────────────────────────────────────────────────────┐   │
│    │  csp_send(conn, packet)                                            │   │
│    │  - conn->idout contains: src, dst, sport, dport, flags             │   │
│    └────────────────────────────────────────────────────────────────────┘   │
│                                      │                                       │
│                                      ▼                                       │
│    Routing Layer                                                             │
│    ┌────────────────────────────────────────────────────────────────────┐   │
│    │  csp_send_direct(&conn->idout, packet, NULL)                       │   │
│    │  - Finds route: interface + optional via address                   │   │
│    │  - Applies source address from outgoing interface if src==0        │   │
│    └────────────────────────────────────────────────────────────────────┘   │
│                                      │                                       │
│                                      ▼                                       │
│    Interface Layer                                                           │
│    ┌────────────────────────────────────────────────────────────────────┐   │
│    │  csp_send_direct_iface(idout, packet, iface, via, from_me)         │   │
│    │  - Appends HMAC/CRC32 if flags set (only if from_me)               │   │
│    │  - Calls iface->nexthop(iface, via, packet, from_me)               │   │
│    └────────────────────────────────────────────────────────────────────┘   │
│                                      │                                       │
│                                      ▼                                       │
│    Physical Layer (e.g., CAN)                                                │
│    ┌────────────────────────────────────────────────────────────────────┐   │
│    │  csp_can1_tx(iface, via, packet, from_me)                          │   │
│    │  - dest = (via != CSP_NO_VIA_ADDRESS) ? via : packet->id.dst       │   │
│    │  - CAN frame addressed to 'dest' (link-layer)                      │   │
│    │  - CSP header inside frame still contains original src/dst         │   │
│    └────────────────────────────────────────────────────────────────────┘   │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘

Example: Node 1 sends to Node 193 via gateway Node 2

┌──────────────┐     ┌──────────────┐     ┌──────────────┐
│   Node 1     │     │   Node 2     │     │   Node 193   │
│   (OBC)      │     │   (Radio)    │     │   (Ground)   │
└──────┬───────┘     └──────┬───────┘     └──────┬───────┘
       │                    │                    │
       │ CAN Frame          │                    │
       │ Link-layer: dst=2  │                    │
       │ CSP header:        │                    │
       │   src=1, dst=193   │                    │
       │ ──────────────────→│                    │
       │                    │                    │
       │                    │ RF Frame           │
       │                    │ Link-layer: N/A    │
       │                    │ CSP header:        │
       │                    │   src=1, dst=193   │
       │                    │ ──────────────────→│
       │                    │                    │✓ Delivered
```

---

## Summary

| Topic | Key Points |
|-------|------------|
| **Route Priority** | 1. Loopback → 2. Subnet match → 3. Routing table → 4. Default interfaces |
| **Longest Match** | Routes with larger netmask (more specific) win |
| **Split Horizon** | Never send back to source interface or same subnet |
| **VIA Address** | Link-layer destination; CSP header keeps original dst |
| **Source Address** | Auto-assigned from outgoing interface if `src==0` and `from_me==1` |
| **Multi-Interface** | Same address allowed on multiple interfaces for redundancy |
| **Interface Selection** | No direct API; determined by routing configuration |

---

## Code References

| Function | File | Line | Purpose |
|----------|------|------|---------|
| `csp_route_work()` | src/csp_route.c | 131-321 | Main routing loop |
| `csp_send_direct()` | src/csp_io.c | 87-217 | Interface selection and forwarding |
| `csp_send_direct_iface()` | src/csp_io.c | 225-283 | Send via specific interface |
| `csp_rtable_find_route()` | src/csp_rtable_cidr.c | 44-74 | Longest-prefix route lookup |
| `csp_iflist_get_by_subnet()` | src/csp_iflist.c | 33-61 | Find interfaces matching subnet |
| `csp_iflist_is_within_subnet()` | src/csp_iflist.c | 15-31 | Check if address in subnet |
| `csp_id_is_broadcast()` | src/csp_id.c | 260-271 | Check for broadcast address |

