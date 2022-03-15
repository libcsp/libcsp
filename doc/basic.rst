The basics of CSP
=================

The following diagram shows a conceptual overview of the different blocks in CSP. The shown inferface is CAN (src/interfaces/csp_if_can.c, driver: src/drivers/can/can_socketcan.c).

.. code-block:: none

           buffer                  connection   send   read/accept
              ^                         |         |         ^
              |                         |         v         |
              |   +-----------------+   |     +----------------+    +-------------------+
              |   | connection pool |   |     |    CSP Core    |    | csp_task_router() |
              |   |  csp_conn.c     |<--+---->| * routing      |<---|    csp_route.c    |
              |   +-----------------+         | * crypt        |    +-------------------+
              |                               | * UDP/RDP      |              ^
              |                  csp_route_t  | * CRC32        |              |
              v                 +------------>|                |     +----------------+
       +--------------+         |             |                |     | incoming queue |
       | buffer pool  |         |             |                |     |     qfifo.c    |
       | csp_buffer.c |  +-------------+      +----------------+     +----------------+
       +--------------+  |routing table|            |                         ^
              ^          |  rtable.c   |            v                         |
              |          +-------------+      +------------------------------------+
              |                               |  (next_hop)                        |
              |                               |    CAN interface (csp_if_can.c)    |
              +------------------------------>|                      csp_can_rx()  |
                                              +------------------------------------+
                                                    |   CAN frame (8 bytes)   ^
                                                    v                         |
                                            csp_can_tx_frame()       socketcan_rx_thread()
					            (drivers/can/can_socketcan.c)


Buffer
------

All buffers are allocated once during initialization of CSP, after this the buffer system is entirely self-contained. All allocated elements are of the same size, so the buffer size must be chosen to be able to handle the maximum possible packet length. The buffer pool uses a queue to store pointers to free buffer elements. First of all, this gives a very quick method to get the next free element since the dequeue is an O(1) operation. Furthermore, since the queue is a protected operating system primitive, it can be accessed from both task-context and interrupt-context. The `csp_buffer_get()` version is for task-context and `csp_buffer_get_isr()` is for interrupt-context. Using fixed size buffer elements that are preallocated is again a question of speed and safety.

Definition of a buffer element `csp_packet_t`:

.. literalinclude:: ../include/csp/csp_types.h
   :language: c
   :start-after: //doc-begin:csp_packet_t
   :end-before: //doc-end:csp_packet_t

A basic concept in the buffer system is called Zero-Copy. This means that from userspace to the kernel-driver, the buffer is never copied from one buffer to another. This is a big deal for a small microprocessor, where a call to `memcpy()` can be very expensive.
This is achieved by a number of `padding` bytes in the buffer, allowing for a header to be prepended at the lower layers without copying the actual payload. This also means that there is a strict contract between the layers, which data can be modified and where.

The padding bytes are used by the I2C interface, where the `csp_packet_t` will be casted to a `csp_i2c_frame_t`, when the interface calls the driver Tx function `csp_i2c_driver_tx_t`:

.. literalinclude:: ../include/csp/interfaces/csp_if_i2c.h
   :language: c
   :start-after: //doc-begin:csp_i2c_frame_t
   :end-before: //doc-end:csp_i2c_frame_t


Connection
----------

CSP supports both connection-less and connection-oriented connections. See more about protocols in :ref:`layer4`.

During initialization libcsp allocates the configured number of connections. The required number of connections depends on the application. Here is a list functions, that will allocate a connection from the connection pool:

  - client connection, call to `csp_connect()`
  - server socket for listening `csp_socket()`
  - server accepting an incmoing connection `csp_accept()`

An applications receive queue is located on the connection and is also allocated once during initialization. The length of the queue is the same for all queues, and specified in the configuration.


Send
----

The data flow from the application to the driver, can basically be broken down into following steps:

   1. if using connection-oriented communication, establish a connection> `csp_connect()`, `csp_accept()`
   2. get packet from the buffer pool: `csp_buffer_get()`
   3. add payload data to the packet
   4. send packet, e.g. `csp_send()`, `csp_sendto()`
   5. CSP looks up the destination route, using the routing table, and calls `nexthop()` on the resolved interface.
   6. The interface (in this case the CAN interface), splits the packet into a number of CAN frames (8 bytes) and forwards them to the driver.


Receive
-------

The data flow from the driver to the application, can basically be broken down into following steps:

   1. the driver layer forwards the raw data frames to the interface, in this case CAN frames
   2. the interface will aquire a free buffer (e.g. `csp_buffer_get_isr()`) for assembling the CAN frames into a complete packet
   3. once the interface has successfully assembled a packet, the packet is queued for routing - primarily to decouple the interface, e.g. if the interfacec/drivers uses interrupt (ISR).
   4. the router picks up the packet from the incoming queue and routes it on - this can either to a local destination, or another interface.
   5. the application waits for new packets at its Rx queue, by calling `csp_read()` or `csp_accept` in case it is a server socket.
   6. the application can now process the packet, and either send it using e.g. `csp_send()`, or free the packet using `csp_buffer_free()`.


Routing table
-------------

When a packet is routed, the destination address is looked up in the routing table, which results in a `csp_route_t` record. The record contains the inteface (`csp_iface_t`) the packet is to be send on, and an optional `via` address. The `via` address is used, when the sender cannot direcly reach the receiver on one of its connected networks, e.g. sending a packet from the satellite to the ground - the radio will be the `via` address.

CSP comes with 2 routing table implementations (selected at compile time).

 - static: supports a one-to-one mapping, meaning routes must be configured per destination address or a single `default` address. The `default` address is used, in case there are no routes set for the specific destination address.
   The `static` routing table has the fastest lookup, but requires more setup.

 - cidr (Classless Inter-Domain Routing): supports a one-to-many mapping, meaning routes can be configued for a range of destianation addresses.
   The `cidr` is a bit slower for lookup, but simple to setup.

Routes can be configured using text strings in the format:

   <address>[/mask] <interface name> [via]

   * address: is the destination address, the routing table will match it against the CSP header destination.
   * mask (optional): determines how many MSB bits of address are to be matched. mask = 1 will only match the MSB bit, mask = 2 will match 2 MSB bits. Mask values different from 0 and 5, is only supported by the cidr rtable.
   * interface name: name of the interface to route the packet on
   * via (optional) address: if different from 255, route the packet to the `via` address, instead of the address in the CSP header.

Here are some examples:

 - "10 I2C" route destination address 10 to the "I2C" interface and send it to address 10 (no `via`).
 - "10 I2C 30" route destination address 10 to the "I2C" interface and send it to address 30 (`via`). The original destination address 10 is not changed in the CSP header of the packet.
 - "16/1 CAN 4" (CIDR only) route all destinations addresses 16-31 to address 4 on the CAN interface.
 - "0/0 CAN" default route, if no other matching route is found, route packet onto the CAN interface.


Interface
---------

The interface typically implements :ref:`layer2`, and uses drivers from :ref:`layer1` to send/receive data.
The interface is a generic struct, with no knowledge of any specific interface , protocol or driver:

.. literalinclude:: ../include/csp/csp_interface.h
   :language: c
   :start-after: //doc-begin:csp_iface_s
   :end-before: //doc-end:csp_iface_s

If an interface implementation needs to store data, e.g. state information (KISS), it can use the pointer `interface_data` to reference any data structure needed. The driver implementation can use the pointer `driver_data` for storing data, e.g. device number.

See function `csp_can_socketcan_open_and_add_interface()` in `src/drivers/can/can_socketcan.c` for an example of how to implement a CAN driver and hooking it into CSP, using the CSP standard CAN interface.

Send
^^^^

When CSP needs to send a packet, it calls `nexthop` on the interface returned by route lookup.
If the interface succeeds in sending the packet, it must free the packet.
In case of failure, the packet must not be freed by the interface. The original idea was, that the packet could be retried later on, without having to re-create the packet again. However, the current implementation does not yet fully support this as some interfaces modifies header (endian conversion) or data (adding CRC32).

Receive
^^^^^^^

When receiving data, the driver calls into the interface with the received data, e.g. `csp_can_rx()`. The interface will convert/copy the data into a packet (e.g. by assembling all CAN frames). Once a complete packet is received, the packet is  queued for later CSP processing, by calling `csp_qfifo_write()`.

