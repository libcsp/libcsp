# CSP v2 Satellite Network Topology Assignment

## Overview

This document defines the CSP v2 (14-bit addressing) network topology for a satellite mission with ground segment connectivity.

## Network Architecture

```
╔══════════════════════════════════════════════════════════════════════════════╗
║                     SATELLITE MISSION CSP v2 NETWORK TOPOLOGY                 ║
╠══════════════════════════════════════════════════════════════════════════════╣
║                                                                               ║
║  ┌─────────────────────────────────────────────────────────────────────────┐ ║
║  │                         SPACE SEGMENT                                    │ ║
║  │                                                                          │ ║
║  │   ╔═══════════════════════════ MAIN CAN BUS (256/10) ════════════════╗  │ ║
║  │   ║                                                                   ║  │ ║
║  │   ║  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌─────┐ ║  │ ║
║  │   ║  │ RADIO_A  │  │ RADIO_B  │  │   OBC    │  │   AOCS   │  │ EPS │ ║  │ ║
║  │   ║  │ Addr:256 │  │ Addr:257 │  │ Addr:258 │  │ Addr:259 │  │ 260 │ ║  │ ║
║  │   ║  │ [ROUTER] │  │ [ROUTER] │  │ [ROUTER] │  │ [ROUTER] │  │     │ ║  │ ║
║  │   ║  └────┬─────┘  └────┬─────┘  └────┬─────┘  └────┬─────┘  └─────┘ ║  │ ║
║  │   ║       │RF           │RF           │SPI          │I2C             ║  │ ║
║  │   ╚═══════╪═════════════╪═════════════╪═════════════╪════════════════╝  │ ║
║  │           │             │             │             │                    │ ║
║  │           │             │      ┌──────┴──────┐  ┌───┴───────────┐       │ ║
║  │           │             │      │OBC SPI BUS  │  │AOCS I2C BUS   │       │ ║
║  │           │             │      │  (320/10)   │  │  (384/10)     │       │ ║
║  │           │             │      ├─────────────┤  ├───────────────┤       │ ║
║  │           │             │      │PAYLOAD_0:321│  │RW_0: 385      │       │ ║
║  │           │             │      │PAYLOAD_1:322│  │RW_1: 386      │       │ ║
║  │           │             │      │PAYLOAD_2:323│  │RW_2: 387      │       │ ║
║  │           │             │      │PAYLOAD_3:324│  │RW_3: 388      │       │ ║
║  │           │             │      └─────────────┘  └───────────────┘       │ ║
║  └───────────┼─────────────┼────────────────────────────────────────────────┘ ║
║              │             │                                                  ║
║              │    RF LINK  │                                                  ║
║              └──────┬──────┘                                                  ║
║                     │                                                         ║
║  ┌──────────────────┼──────────────────────────────────────────────────────┐ ║
║  │                  │            GROUND SEGMENT                             │ ║
║  │            ┌─────┴─────┐                                                 │ ║
║  │            │    TNC    │     Ground Station (512/10 subnet)              │ ║
║  │            │ Addr: 512 │                                                 │ ║
║  │            │ [ROUTER]  │                                                 │ ║
║  │            └───────────┘                                                 │ ║
║  └──────────────────────────────────────────────────────────────────────────┘ ║
╚══════════════════════════════════════════════════════════════════════════════╝
```

## Address Allocation

| Subnet    | CIDR   | Range     | Capacity | Nodes                                    |
|-----------|--------|-----------|----------|------------------------------------------|
| Main CAN  | 256/10 | 256-271   | 16       | RADIO_A, RADIO_B, OBC, AOCS, EPS         |
| OBC SPI   | 320/10 | 320-335   | 16       | OBC(gateway), PAYLOAD_0-3                |
| AOCS I2C  | 384/10 | 384-399   | 16       | AOCS(gateway), RW_0-3                    |
| Ground    | 512/10 | 512-527   | 16       | TNC                                      |

### Node Address Table

| Node      | Address | Interface(s)          | Role           |
|-----------|---------|----------------------|----------------|
| RADIO_A   | 256     | CAN0(256), RF0(256)  | Router/Gateway |
| RADIO_B   | 257     | CAN0(257), RF0(257)  | Router/Backup  |
| OBC       | 258     | CAN0(258), SPI0(320) | Router         |
| AOCS      | 259     | CAN0(259), I2C0(384) | Router         |
| EPS       | 260     | CAN0(260)            | Endpoint       |
| PAYLOAD_0 | 321     | SPI0(321)            | Endpoint       |
| PAYLOAD_1 | 322     | SPI0(322)            | Endpoint       |
| PAYLOAD_2 | 323     | SPI0(323)            | Endpoint       |
| PAYLOAD_3 | 324     | SPI0(324)            | Endpoint       |
| RW_0      | 385     | I2C0(385)            | Endpoint       |
| RW_1      | 386     | I2C0(386)            | Endpoint       |
| RW_2      | 387     | I2C0(387)            | Endpoint       |
| RW_3      | 388     | I2C0(388)            | Endpoint       |
| TNC       | 512     | RF0(512)             | Router/Ground  |

## Routing Tables

### RADIO_A (Primary Ground Gateway)
```
256/10  CAN0              # Main CAN subnet (direct)
320/10  CAN0 via 258      # OBC SPI subnet via OBC
384/10  CAN0 via 259      # AOCS I2C subnet via AOCS
512/10  RF0               # Ground subnet (direct)
```

### RADIO_B (Redundant Gateway)
```
256/10  CAN0              # Main CAN subnet (direct)
320/10  CAN0 via 258      # OBC SPI subnet via OBC
384/10  CAN0 via 259      # AOCS I2C subnet via AOCS
512/10  RF0               # Ground subnet (direct)
```

### OBC (CAN ↔ SPI Router)
```
256/10  CAN0              # Main CAN subnet (direct)
320/10  SPI0              # OBC SPI subnet (direct)
384/10  CAN0 via 259      # AOCS I2C subnet via AOCS
512/10  CAN0 via 256      # Ground via Radio A
```

### AOCS (CAN ↔ I2C Router)
```
256/10  CAN0              # Main CAN subnet (direct)
384/10  I2C0              # AOCS I2C subnet (direct)
320/10  CAN0 via 258      # OBC SPI subnet via OBC
512/10  CAN0 via 256      # Ground via Radio A
```

### EPS, PAYLOAD_0-3, RW_0-3 (Endpoints)
```
0/0     <iface> via <gw>  # Default route to respective gateway
```
- EPS: `0/0 CAN0 via 256` (Radio A)
- PAYLOAD_x: `0/0 SPI0 via 320` (OBC)
- RW_x: `0/0 I2C0 via 384` (AOCS)

### TNC (Ground Station)
```
256/10  RF0 via 256       # Main CAN via Radio A
320/10  RF0 via 256       # OBC SPI via Radio A
384/10  RF0 via 256       # AOCS I2C via Radio A
512/10  RF0               # Ground local (direct)
```

## CSP v2 Implementation Notes

### CAN Bus Behavior
- **Broadcast Medium**: All nodes on the CAN bus receive every frame
- **Hardware Filtering**: Nodes accept frames based on CFP destination field match
- **Via Parameter**: Ignored at physical layer in CSP v2 (unlike CSP 1.x)
- **Link Destination**: Always uses `packet.id.dst` for CAN frame addressing

### Routing Priority (per `csp_rtable_cidr.c`)
1. **Loopback**: If `dst == local_address`
2. **Subnet Match**: Interface where destination falls within interface subnet
3. **Routing Table**: Longest-prefix match (most specific netmask wins)
4. **Default Interface**: Fallback if no route found

### Split-Horizon Rules (per `csp_io.c`)
- Never forward packet back on the same interface it arrived on
- Never forward packet to a subnet containing the packet's source address
- Prevents routing loops in multi-homed configurations

## Verification Results

All 18 bidirectional reachability tests pass:

| Test Case | Source → Destination | Result |
|-----------|---------------------|--------|
| Ground→Space | TNC → RADIO_A/B, OBC, AOCS, EPS | ✓ PASS |
| Ground→Payload | TNC → PAYLOAD_0-3 | ✓ PASS |
| Ground→RW | TNC → RW_0-3 | ✓ PASS |
| Space→Ground | OBC, AOCS, EPS → TNC | ✓ PASS |
| Payload→Ground | PAYLOAD_0 → TNC | ✓ PASS |
| RW→Ground | RW_0 → TNC | ✓ PASS |
| Inter-node | OBC ↔ AOCS, OBC → RW, AOCS → PAYLOAD | ✓ PASS |

## Simulation Tool

Run the topology simulator to verify routing:

```bash
python3 csp_topology_simulator.py
```

The simulator validates:
- Address allocation algorithm
- Routing table correctness
- Packet delivery across all interface types (CAN, SPI, I2C, RF)
- Split-horizon enforcement
- Bidirectional reachability

