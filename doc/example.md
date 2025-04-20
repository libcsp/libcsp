# Client and server example

This page explains how to build and run the example programs provided with `libcsp`.

## Building the examples

To compile all the examples, simply run:

`./examples/buildall.py`

This script builds all the example binaries using your system’s default compiler and 
places the resulting executables in the `build/examples/` directory.

## Available Example Programs

The following example programs are included:

- **`csp_server`**  
  A server that listens for incoming packets.

- **`csp_client`**  
  A client that sends packets to a specified destination.

- **`zmqproxy`**  
  A simple router (or "hub") that connects multiple ZMQ interfaces. (use for zmqhub client and server example)

- **`csp_bridge_can2udp`**  
  A bridge that converts CAN packets to UDP packets

## Available drivers and interfaces examples

The example supports these drivers and interfaces in CSP:

  - ZMQHUB: `-z <host name|ip>`

    Requires no extra hardware, as it uses standard network. The
    `zmqproxy` will need to be started.

  - CAN: `-c <can device>`

    Requires a physical CAN interface. There are several CAN dongles on
    the market, for example
    <https://www.peak-system.com/PCAN-USB.199.0.html>.

    To achieve best performance and stability, following options can be
    set on the CAN device:

    ```
    linux: sudo ip link set dev can0 down
    linux: sudo ip link set dev can0 up type can bitrate 1000000 restart-ms 100
    linux: sudo ip link set dev can0 txqueuelen 100
    ```

  - KISS: `-k <serial device>`

    Requires a serial interface, e.g. USB dongle.

  - ETH: `-e <ethernet interface>`
    Raw Ethernet driver. Requires specifying a physical interface name (e.g. eth0).

  - UDP: `-u <udp address>`
    Sends/receives CSP frames over UDP.

## Running the client and server examples

The csp_server and csp_client programs are basic tools for sending and receiving packets over CSP.
They can use different interfaces such as ZMQ, CAN, KISS, ETH, UDP depending on the options provided.

## Running the example with ZMQHUB interface

To run the example with ZMQHUB interfaces, start the `zmqproxy`, client and server in three separate processes.

    libcsp$ ./build/examples/zmqproxy
    Subscriber task listening on tcp://0.0.0.0:6000
    Publisher task listening on tcp://0.0.0.0:7000
    Capture/logging task listening on tcp://0.0.0.0:6000
    Packet: Src 3, Dst 2, Dport 1, Sport 18, Pri 2, Flags 0x00, Size 100
    Packet: Src 2, Dst 3, Dport 18, Sport 1, Pri 2, Flags 0x00, Size 100
    Packet: Src 3, Dst 2, Dport 4, Sport 19, Pri 2, Flags 0x01, Size 8
    Packet: Src 3, Dst 2, Dport 10, Sport 20, Pri 2, Flags 0x00, Size 14

    libcsp$ ./build/examples/csp_server -z localhost -a 2
    Initialising CSP
    Connection table
    [00 0x7f266ec1d080] S:0, 0 -> 0, 0 -> 0 (17) fl 0
    [01 0x7f266ec1d198] S:0, 0 -> 0, 0 -> 0 (18) fl 0
    [02 0x7f266ec1d2b0] S:0, 0 -> 0, 0 -> 0 (19) fl 0
    [03 0x7f266ec1d3c8] S:0, 0 -> 0, 0 -> 0 (20) fl 0
    [04 0x7f266ec1d4e0] S:0, 0 -> 0, 0 -> 0 (21) fl 0
    [05 0x7f266ec1d5f8] S:0, 0 -> 0, 0 -> 0 (22) fl 0
    [06 0x7f266ec1d710] S:0, 0 -> 0, 0 -> 0 (23) fl 0
    [07 0x7f266ec1d828] S:0, 0 -> 0, 0 -> 0 (24) fl 0
    Interfaces
    LOOP      addr: 0 netmask: 14 dfl: 0
              tx: 00000 rx: 00000 txe: 00000 rxe: 00000
              drop: 00000 autherr: 00000 frame: 00000
              txb: 0 (0B) rxb: 0 (0B)

    ZMQHUB    addr: 2 netmask: 0 dfl: 1
              tx: 00000 rx: 00000 txe: 00000 rxe: 00000
              drop: 00000 autherr: 00000 frame: 00000
              txb: 0 (0B) rxb: 0 (0B)

    Server task started
    Packet received on SERVER_PORT: Hello world A

    libcsp$ ./build/examples/csp_client -z localhost -a 3 -C 2
    Initialising CSP
    Connection table
    [00 0x7f471efc1080] S:0, 0 -> 0, 0 -> 0 (17) fl 0
    [01 0x7f471efc1198] S:0, 0 -> 0, 0 -> 0 (18) fl 0
    [02 0x7f471efc12b0] S:0, 0 -> 0, 0 -> 0 (19) fl 0
    [03 0x7f471efc13c8] S:0, 0 -> 0, 0 -> 0 (20) fl 0
    [04 0x7f471efc14e0] S:0, 0 -> 0, 0 -> 0 (21) fl 0
    [05 0x7f471efc15f8] S:0, 0 -> 0, 0 -> 0 (22) fl 0
    [06 0x7f471efc1710] S:0, 0 -> 0, 0 -> 0 (23) fl 0
    [07 0x7f471efc1828] S:0, 0 -> 0, 0 -> 0 (24) fl 0
    Interfaces
    LOOP      addr: 0 netmask: 14 dfl: 0
              tx: 00000 rx: 00000 txe: 00000 rxe: 00000
              drop: 00000 autherr: 00000 frame: 00000
              txb: 0 (0B) rxb: 0 (0B)

    ZMQHUB    addr: 3 netmask: 0 dfl: 1
              tx: 00000 rx: 00000 txe: 00000 rxe: 00000
              drop: 00000 autherr: 00000 frame: 00000
              txb: 0 (0B) rxb: 0 (0B)

    Client task started
    Ping address: 2, result 8 [mS]
    reboot system request sent to address: 2
