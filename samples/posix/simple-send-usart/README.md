# Simple Send via USART

This is a simple sample code to send `"abc"` to a server via the USART
interface.

## How to Build

```
$ cmake -B builddir
$ ninja -C builddir simple-send-usart csp_server
```

## How to Test

To run without UART/USART, you can setup a pair of PTY on Linux:

```
$ socat -dd pty,raw,echo=0,link=/tmp/pty1 pty,raw,echo=0,link=/tmp/pty2
```

This command will create two device files `/tmp/pty1` and `/tmp/pty2`
for a server and client respectively.

You’ll need the socat command. If you don't have it, install it with:

```
$ sudo apt-get install socat
```

First, you need to run a CSP server:

```
$ ./builddir/examples/csp_server -k /tmp/pty1 -a 1
Initialising CSP
Connection table
[00 0x7f1332d208e0] S:0, 0 -> 0, 0 -> 0 (17) fl 0
[01 0x7f1332d209f8] S:0, 0 -> 0, 0 -> 0 (18) fl 0
[02 0x7f1332d20b10] S:0, 0 -> 0, 0 -> 0 (19) fl 0
[03 0x7f1332d20c28] S:0, 0 -> 0, 0 -> 0 (20) fl 0
[04 0x7f1332d20d40] S:0, 0 -> 0, 0 -> 0 (21) fl 0
[05 0x7f1332d20e58] S:0, 0 -> 0, 0 -> 0 (22) fl 0
[06 0x7f1332d20f70] S:0, 0 -> 0, 0 -> 0 (23) fl 0
[07 0x7f1332d21088] S:0, 0 -> 0, 0 -> 0 (24) fl 0
Interfaces
LOOP       addr: 0 netmask: 14 dfl: 0
           tx: 00000 rx: 00000 txe: 00000 rxe: 00000
           drop: 00000 autherr: 00000 frame: 00000
           txb: 0 (0B) rxb: 0 (0B)

KISS       addr: 1 netmask: 0 dfl: 1
           tx: 00000 rx: 00000 txe: 00000 rxe: 00000
           drop: 00000 autherr: 00000 frame: 00000
           txb: 0 (0B) rxb: 0 (0B)

Server task started
```

Then, in another terminal, run `simple-send-usart`

```
./builddir/samples/posix/simple-send-usart/simple-send-usart
```

If it runs successfully, you’ll see a new message on the server
terminal:

```
Packet received on SERVER_PORT: abc
```
