# Simple Send via CANbus

This is a simple sample code to send `"abc"` to a server via the
CANbus interface.

## How to Build

```
$ cmake -B builddir
$ ninja -C builddir simple-send-canbus csp_server
```

## How to Test

To run without a real CANbus hardware, you can setup a vcan interface
on Linux:

```
$ sudo ip link add dev vcan0 type vcan
$ sudo ip link set up vcan0
```

This command will create a virtual CANbus interface that a server and
a client can share.

You’ll need the `ip` command. If you don't have it, install it with:

```
$ sudo apt-get install iproute2
```

First, you need to run a CSP server:

```
$ ./builddir/examples/csp_server -c vcan0 -a 1
Initialising CSP
INIT CAN: device: [vcan0], bitrate: 1000000, promisc: 1
RTNETLINK answers: Operation not permitted
RTNETLINK answers: Operation not permitted
RTNETLINK answers: Operation not permitted
RTNETLINK answers: Operation not permitted
Connection table
[00 0x7fbfc89208e0] S:0, 0 -> 0, 0 -> 0 (17) fl 0
[01 0x7fbfc89209f8] S:0, 0 -> 0, 0 -> 0 (18) fl 0
[02 0x7fbfc8920b10] S:0, 0 -> 0, 0 -> 0 (19) fl 0
[03 0x7fbfc8920c28] S:0, 0 -> 0, 0 -> 0 (20) fl 0
[04 0x7fbfc8920d40] S:0, 0 -> 0, 0 -> 0 (21) fl 0
[05 0x7fbfc8920e58] S:0, 0 -> 0, 0 -> 0 (22) fl 0
[06 0x7fbfc8920f70] S:0, 0 -> 0, 0 -> 0 (23) fl 0
[07 0x7fbfc8921088] S:0, 0 -> 0, 0 -> 0 (24) fl 0
Interfaces
LOOP       addr: 0 netmask: 14 dfl: 0
           tx: 00000 rx: 00000 txe: 00000 rxe: 00000
           drop: 00000 autherr: 00000 frame: 00000
           txb: 0 (0B) rxb: 0 (0B)

CAN        addr: 1 netmask: 0 dfl: 1
           tx: 00000 rx: 00000 txe: 00000 rxe: 00000
           drop: 00000 autherr: 00000 frame: 00000
           txb: 0 (0B) rxb: 0 (0B)

Server task started
```

Then, in another terminal, run `simple-send-canbus`

```
$ ./builddir/samples/posix/simple-send-canbus/simple-send-canbus
INIT CAN: device: [vcan0], bitrate: 1000000, promisc: 1
RTNETLINK answers: Operation not permitted
RTNETLINK answers: Operation not permitted
RTNETLINK answers: Operation not permitted
RTNETLINK answers: Operation not permitted
```

You see warnings like `RTNETLINK answers: Operation not permitted`
because your are not root. It should work without using `sudo`.  Also
even you use `sudo`, you still see a few warnings because `vcan0` does
not support a few operations.

If you successfully run `simple-send-canbus`, you see the following
message on the server terminal.

```
Packet received on SERVER_PORT: abc
```
