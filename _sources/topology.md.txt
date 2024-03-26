# Network Topology

CSP uses a network oriented terminology similar to what is known from
the Internet and the IP model. A CSP network can be configured for
several different topologies.

```
  +-------+-------+-------+-------+       bus: I2C, CAN, KISS
  |       |       |       |       |
+---+   +---+   +---+   +---+   +---+
|OBC|   |COM|   |EPS|   |PL1|   |PL2|     Nodes 128/8 (Space segment)
+---+   +---+   +---+   +---+   +---+
          ^
          |  Radio                        bus: RF
          v                               Nodes 192/12
        +---+          +----+
        |TNC|----------| PC |             bus: KISS, CAN, ETH
        +---+          +----+             Nodes: 196/12 (Space segment)
                          |       
                          |               
                          |               bus: ZMQ
                  +-----------+           Nodes: 96/9 (Ground segment)
                  |           |
                +----+     +----+
                | PC |     | PC |
                +----+     +----+
```                        

| CSP v2 subnet mask  | 4    | 5   | 6   | 7   | 8  | 9  | 10 | 11 | 12 |
| --------------------|------|-----|-----|-----|----|----|----|----|----|
| Available addresses | 1023 | 511 | 255 | 127 | 63 | 31 | 15 | 7  | 3  | 

For CSP v2, addresses from 1 to 16383 are available, with subnet sizes
typically varying from 2²=4 to 2⁶=64. Address 0 is always reserved for
localhost. Each address space has its highest address defined as a
broadcast address, making the number of available addresses one less than
the subnet size. Each interface in a sub-system  has an independent address
belonging to the particular logical interface. 

A network is configured using static routes for every module, initialised
at boot-up of each sub-system. The routing is configured by defining whether
each interface in a sub-system is a default interface to use when a
receiver address is not covered by any interface. On top of that, static
routes can be defined to cover complex network setups with multiple
satellites, multiple subnets in a satellite and ground segments split into
multiple subnets.

Each physical network (a CAN bus, RF link, etc.) will have an independent
logical network related, unless two interfaces are used to create a redundant 
physical network in a satellite or spacelink. In this case, the two (or more)
physical interfaces in the sub-systems shall be configured to have the same
CSP address and related properties, by which the CSP router knows that packets
for that particular logical CSP subnet shall be transmitted on both physical
interfaces.
