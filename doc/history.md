# History

The idea was developed by a group of students from Aalborg University
in 2008. In 2009 the main developer started working for GomSpace, and CSP
became integrated into the GomSpace products. The same developer later
moved to Space Inventor, at which CSP was further developed.

The three letter acronym CSP was originally an abbreviation for CAN
Space Protocol because the first MAC-layer driver was written for
CAN-bus. Now the physical layer has extended to include SpaceWire, I2C
and RS232, the name was therefore extended to the more general CubeSat
Space Protocol without changing the abbreviation.

## CSP versions

CSP was originally developed for a 1U cubesat, containing only the minimum
required number of modules for a distributed satellite configuration. At
that time, the address space was therefore decided to handle a maximum of
32 nodes, split between the satellite and ground segment.

As the protocol developed and was used on larger satellite projects and
multi-satellite constellations, a need for a larger address space was required.

CSP version 2 was therefore specified, increasing the address range to 14 bits, 
corresponding to 16,384 addresses. The CSP implementation is runtime 
configurable to run as CSP version or version 2, but version 1 and version 2
modules are not supposed to be on the same network infrastructure.

## Satellites using CSP

Here is a list of some of the known satellites or organisations, that
uses CSP:

  - GomSpace GATOSS GOMX-1
  - AAUSAT3
  - EgyCubeSat
  - EuroLuna
  - NUTS
  - Hawaiian Space Flight Laboratory
  - GomSpace GOMX-3, GOMX-4 A & B
  - Sternula
  - G-Space 1

See list of libcsp fork's [here](https://github.com/libcsp/libcsp/network/members)
