Client and server example
=========================

The example in `example/csp_server_client.c` provides a simple server/client setup, where the client sends a request to the server and receives a reply. The code can be compiled to an executable using `./examples/buildall.py`.

The example supports the drivers and interfaces in CSP:

 - ZMQHUB: -z <host name|ip>

   Requires no extra hardware, as it uses standard network. The zmqproxy will need to be started.

 - CAN: -c <can device>

   Requires a physical CAN interface. There are several CAN dongles on the market, for example https://www.peak-system.com/PCAN-USB.199.0.html.

   To achieve best performance and stabilty, following options can be set on the CAN device:

      .. code-block:: none

	 linux: sudo ip link set dev can0 down
	 linux: sudo ip link set dev can0 up type can bitrate 1000000 restart-ms 100
	 linux: sudo ip link set dev can0 txqueuelen 100

 - KISS: -k <serial device>

   Requires a serial interface, e.g. USB dongle.


Running the example
^^^^^^^^^^^^^^^^^^^
If the example is started without any interfaces, it will use the loopback interface for communication between client and server::

  ubuntu-18:~/libcsp$ ./build/csp_server_client 
  1586816581.410181 Initialising CSP
  Connection table
  [00 0x55a00f7adee0] S:0, 0 -> 0, 0 -> 0, sock: (nil)
  [01 0x55a00f7adf68] S:0, 0 -> 0, 0 -> 0, sock: (nil)
  [02 0x55a00f7adff0] S:0, 0 -> 0, 0 -> 0, sock: (nil)
  [03 0x55a00f7ae078] S:0, 0 -> 0, 0 -> 0, sock: (nil)
  [04 0x55a00f7ae100] S:0, 0 -> 0, 0 -> 0, sock: (nil)
  [05 0x55a00f7ae188] S:0, 0 -> 0, 0 -> 0, sock: (nil)
  [06 0x55a00f7ae210] S:0, 0 -> 0, 0 -> 0, sock: (nil)
  [07 0x55a00f7ae298] S:0, 0 -> 0, 0 -> 0, sock: (nil)
  [08 0x55a00f7ae320] S:0, 0 -> 0, 0 -> 0, sock: (nil)
  [09 0x55a00f7ae3a8] S:0, 0 -> 0, 0 -> 0, sock: (nil)
  Interfaces
  LOOP       tx: 00000 rx: 00000 txe: 00000 rxe: 00000
             drop: 00000 autherr: 00000 frame: 00000
             txb: 0 (0.0B) rxb: 0 (0.0B) MTU: 0

  Route table
  1/5 LOOP
  0/0 LOOP
  1586816581.410405 Server task started
  1586816581.410453 Binding socket 0x55a00f7adf68 to port 25
  1586816581.410543 Client task started
  1586816582.410983 SERVICE: Ping received
  1586816582.411135 Ping address: 1, result 0 [mS]
  1586816582.411174 reboot system request sent to address: 1
  1586816582.461341 csp_sys_reboot not supported - no user function set
  1586816582.512532 Packet received on MY_SERVER_PORT: Hello World (1)
