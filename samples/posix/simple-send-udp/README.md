# Simple Send via UDP

This is a simple sample code to send `"abc"` to a server via the
UDP interface.

## How to Build

```
$ cmake -B builddir
$ ninja -C builddir simple-send-udp csp_server
```

## How to Test

First, you need to run a CSP server:

```
$ ./builddir/examples/csp_server csp_server -u "127.0.0.1" -a 1
Initialising CSP
UDP peer address: 127.0.0.1:1500 (listening on port 1501)
Connection table
[00 0x745a7ae4e920] S:0, 0 -> 0, 0 -> 0 (17) fl 0
[01 0x745a7ae4ea38] S:0, 0 -> 0, 0 -> 0 (18) fl 0
[02 0x745a7ae4eb50] S:0, 0 -> 0, 0 -> 0 (19) fl 0
[03 0x745a7ae4ec68] S:0, 0 -> 0, 0 -> 0 (20) fl 0
[04 0x745a7ae4ed80] S:0, 0 -> 0, 0 -> 0 (21) fl 0
[05 0x745a7ae4ee98] S:0, 0 -> 0, 0 -> 0 (22) fl 0
[06 0x745a7ae4efb0] S:0, 0 -> 0, 0 -> 0 (23) fl 0
[07 0x745a7ae4f0c8] S:0, 0 -> 0, 0 -> 0 (24) fl 0
Interfaces
LOOP       addr: 0 netmask: 14 dfl: 0
           tx: 00000 rx: 00000 txe: 00000 rxe: 00000
           drop: 00000 autherr: 00000 frame: 00000
           txb: 0 (0B) rxb: 0 (0B) 

UDP        addr: 5 netmask: 0 dfl: 1
           tx: 00000 rx: 00000 txe: 00000 rxe: 00000
           drop: 00000 autherr: 00000 frame: 00000
           txb: 0 (0B) rxb: 0 (0B) 

Server task started
```

Then, in another terminal, run `simple-send-udp`

```
$ ./builddir/samples/posix/simple-send-udp/simple-send-udp
UDP peer address: 127.0.0.1:1501 (listening on port 1500)
```

If you successfully run `simple-send-udp`, you see the following
message on the server terminal.

```
Packet received on SERVER_PORT: abc
```
