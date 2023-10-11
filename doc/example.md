# Client and server example

The example in `examples/csp_server_client.c` provides a
simple server/client setup, where the client sends a request to the
server and receives a reply. The code can be compiled to an executable
using `./examples/buildall.py`.

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

## Running the example

If the example is started without any interfaces, it will use the
loopback interface for communication between client and server:

    libcsp$ ./build/example/csp_server_client
    Initialising CSP
    Connection table
    [00 0x7fbd8f574080] S:0, 0 -> 0, 0 -> 0 (17) fl 0
    [01 0x7fbd8f574198] S:0, 0 -> 0, 0 -> 0 (18) fl 0
    [02 0x7fbd8f5742b0] S:0, 0 -> 0, 0 -> 0 (19) fl 0
    [03 0x7fbd8f5743c8] S:0, 0 -> 0, 0 -> 0 (20) fl 0
    [04 0x7fbd8f5744e0] S:0, 0 -> 0, 0 -> 0 (21) fl 0
    [05 0x7fbd8f5745f8] S:0, 0 -> 0, 0 -> 0 (22) fl 0
    [06 0x7fbd8f574710] S:0, 0 -> 0, 0 -> 0 (23) fl 0
    [07 0x7fbd8f574828] S:0, 0 -> 0, 0 -> 0 (24) fl 0
    Interfaces
    LOOP       addr: 0 netmask: 14 dfl: 0
               tx: 00000 rx: 00000 txe: 00000 rxe: 00000
               drop: 00000 autherr: 00000 frame: 00000
               txb: 0 (0B) rxb: 0 (0B) 

    Client task started
    Server task started
    Ping address: 0, result 2 [mS]
    reboot system request sent to address: 0
    Packet received on MY_SERVER_PORT: Hello world A
