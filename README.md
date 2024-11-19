![CI Status](https://github.com/libcsp/libcsp/actions/workflows/build-test.yml/badge.svg)

# The Cubesat Space Protocol

![CSP](./doc/_images/csp.png)

Cubesat Space Protocol (CSP) is a small protocol stack written in C. CSP
is designed to ease communication between distributed embedded systems
in smaller networks, such as Cubesats. The design follows the TCP/IP
model and includes a transport protocol, a routing protocol and several
MAC-layer interfaces. The core of `libcsp`
includes a router, a connection oriented socket API and
message/connection pools.

The protocol is based on an very lightweight header containing both transport and
network-layer information. Its implementation is designed for, but not
limited to, embedded systems with very limited CPU and memory resources.
The implementation is written in GNU C and is currently ported to run on FreeRTOS, Zephyr
and Linux (POSIX).

The idea is to give sub-system developers of cubesats the same features
of a TCP/IP stack, but without adding the huge overhead of the IP
header. The small footprint and simple implementation allows a small
8-bit system to be fully connected on the network. This allows all
subsystems to provide their services on the same network level, without
any master node required. Using a service oriented architecture has
several advantages compared to the traditional master/slave topology used
on many cubesats.

  - Standardised network protocol: All subsystems can communicate with
    each other (multi-master)
  - Service loose coupling: Services maintain a relationship that
    minimizes dependencies between subsystems
  - Service abstraction: Beyond descriptions in the service contract,
    services hide logic from the outside world
  - Service reusability: Logic is divided into services with the
    intention of promoting reuse.
  - Service autonomy: Services have control over the logic they
    encapsulate.
  - Service Redundancy: Easily add redundant services to the bus
  - Reduces single point of failure: The complexity is moved from a
    single master node to several well defined services on the network

The implementation of `libcsp` is written
with simplicity in mind, but it's compile time configuration allows it
to have some rather advanced features as well.

## Features

  - Thread safe Socket API
  - Router task with Quality of Services
  - Connection-oriented operation (RFC 908 and 1151).
  - Connection-less operation (similar to UDP)
  - ICMP-like requests such as ping and buffer status.
  - Loopback interface
  - Very Small Footprint in regards to code and memory required
  - Zero-copy buffer and queue system
  - Modular network interface system
  - OS abstraction, currently ported to: FreeRTOS, Zephyr, Linux
  - Broadcast traffic
  - Promiscuous mode

## Documentation

The latest version of the /doc folder is compiled to HTML and hosted on:

  [libcsp.github.io/libcsp/](https://libcsp.github.io/libcsp/)

## Contributing

Thank you for considering contributing to libcsp! We welcome
contributions from the community to help improve and grow the
project. Please take a moment to review our
[guidelines](./doc/contributing.md) before opening a Pull Request!

## Software license

The source code is available under MIT license, see LICENSE for license text
