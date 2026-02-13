# Simple Send via zmqhub

This is a simple sample code to send `"abc"` to a server via zmqhub
interface.

## How to Build

```
$ cmake -B builddir
$ ninja -C builddir simple-send-zmq csp_server zmqproxy
```

## How to Test

You’ll need to install libzmq3. If you don't have it, install it with:

```
$ sudo apt-get install libzmq3-dev
```

First, you need to run a CSP server:

```
$ ./builddir/examples/csp_server -z localhost -a 1
Initialising CSP
Connection table
[00 0x756e9c2f3900] S:0, 0 -> 0, 0 -> 0 (17) fl 0
[01 0x756e9c2f3a18] S:0, 0 -> 0, 0 -> 0 (18) fl 0
[02 0x756e9c2f3b30] S:0, 0 -> 0, 0 -> 0 (19) fl 0
[03 0x756e9c2f3c48] S:0, 0 -> 0, 0 -> 0 (20) fl 0
[04 0x756e9c2f3d60] S:0, 0 -> 0, 0 -> 0 (21) fl 0
[05 0x756e9c2f3e78] S:0, 0 -> 0, 0 -> 0 (22) fl 0
[06 0x756e9c2f3f90] S:0, 0 -> 0, 0 -> 0 (23) fl 0
[07 0x756e9c2f40a8] S:0, 0 -> 0, 0 -> 0 (24) fl 0
Interfaces
LOOP       addr: 0 netmask: 14 dfl: 0
           tx: 00000 rx: 00000 txe: 00000 rxe: 00000
           drop: 00000 autherr: 00000 frame: 00000
           txb: 0 (0B) rxb: 0 (0B) 

ZMQHUB     addr: 1 netmask: 0 dfl: 1
           tx: 00000 rx: 00000 txe: 00000 rxe: 00000
           drop: 00000 autherr: 00000 frame: 00000
           txb: 0 (0B) rxb: 0 (0B) 

Server task started
```

Then, in another terminal, run `zmqproxy`

```
$ ./builddir/examples/zmqproxy
Subscriber task listening on tcp://0.0.0.0:6000
Publisher task listening on tcp://0.0.0.0:7000
Capture/logging task listening on tcp://0.0.0.0:6000
Packet: Src 2, Dst 1, Dport 10, Sport 18, Pri 2, Flags 0x00, Size 4
```

Then, in another terminal, run `simple-send-zmq`

```
./builddir/samples/posix/simple-send-zmq/simple-send-zmq
```

If it runs successfully, you’ll see a new message on the server
terminal:

```
Packet received on SERVER_PORT: abc
```

### Note on `usleep(1000)` in the code

In the `simple-send-zmq` example, a short sleep (`usleep(1000)`) is used before
sending messages.
This delay allows the ZeroMQ sockets enough time to establish their connections
properly.

This is important because, as explained in the official ZeroMQ guide ([ZMQ
Guide, Chapter 1](https://zguide.zeromq.org/docs/chapter1/)):

> *"There is one more important thing to know about PUB-SUB sockets: you do not
know precisely when a subscriber starts to get messages. Even if you start a
subscriber, wait a while, and then start the publisher, the subscriber will
always miss the first messages that the publisher sends. This is because as the
subscriber connects to the publisher (something that takes a small but non-zero
time), the publisher may already be sending messages out."*

In other words, since the connection takes a non-zero amount of time to
establish, sending immediately after connecting may cause the message to be
lost.

Therefore, the small `usleep(1000)` delay helps ensure the message is sent after
the connection is ready.
