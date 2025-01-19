# Simple Send via ETH

This is a simple sample code to send `"abc"` to a server via the
ETH interface.

## How to Build

```
$ cmake -B builddir
$ ninja -C builddir simple-send-eth csp_server
```

## How to Test

You’ll need the `ip` command. If you don't have it, install it with:

```
$ sudo apt-get install iproute2
```

To run without a real ETH hardware, you can setup a veth interface
on Linux:

```
$ sudo ip link add veth0 type veth peer name veth1
$ sudo ip link set veth0 up
$ sudo ip link set veth1 up
```

These commands will create a virtual Ethernet interface (veth0 and veth1) that a server and a client can use to communicate.

Then you will need to prepare the binary with using `setcap` command:

```
$ sudo setcap cap_net_raw+ep ./builddir/examples/csp_server
$ sudo setcap cap_net_raw+ep ./builddir/samples/posix/simple-send-eth/simple-send-eth

```

First, you need to run a CSP server:

```
$ ./builddir/examples/csp_server -e veth1 -a 1
Initialising CSP
INIT ETH veth1 idx 8 node 1 mac 5a:7d:17:a2:33:88
Connection table
[00 0x7ce83b5d0900] S:0, 0 -> 0, 0 -> 0 (17) fl 0
[01 0x7ce83b5d0a18] S:0, 0 -> 0, 0 -> 0 (18) fl 0
[02 0x7ce83b5d0b30] S:0, 0 -> 0, 0 -> 0 (19) fl 0
[03 0x7ce83b5d0c48] S:0, 0 -> 0, 0 -> 0 (20) fl 0
[04 0x7ce83b5d0d60] S:0, 0 -> 0, 0 -> 0 (21) fl 0
[05 0x7ce83b5d0e78] S:0, 0 -> 0, 0 -> 0 (22) fl 0
[06 0x7ce83b5d0f90] S:0, 0 -> 0, 0 -> 0 (23) fl 0
[07 0x7ce83b5d10a8] S:0, 0 -> 0, 0 -> 0 (24) fl 0
Interfaces
LOOP       addr: 0 netmask: 14 dfl: 0
           tx: 00000 rx: 00000 txe: 00000 rxe: 00000
           drop: 00000 autherr: 00000 frame: 00000
           txb: 0 (0B) rxb: 0 (0B)

ETH        addr: 1 netmask: 0 dfl: 1
           tx: 00000 rx: 00000 txe: 00000 rxe: 00000
           drop: 00000 autherr: 00000 frame: 00000
           txb: 0 (0B) rxb: 0 (0B)

Server task started
```

Then, in another terminal, run `simple-send-eth`

```
$ ./build/samples/posix/simple-send-eth/simple-send-eth
INIT ETH veth0 idx 9 node 2 mac 22:49:73:70:7c:81
```

If you successfully run `simple-send-eth`, you see the following
message on the server terminal.

```
Packet received on SERVER_PORT: abc
```
