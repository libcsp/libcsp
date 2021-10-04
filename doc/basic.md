# The basics of CSP

The following diagram shows a conceptual overview of the different
blocks in CSP. The shown inferface is CAN
(src/interfaces/csp\_if\_can.c, driver:
src/drivers/can/can\_socketcan.c).

``` none
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
```

## Buffer

All buffers are allocated once during initialization of CSP, after this
the buffer system is entirely self-contained. All allocated elements are
of the same size, so the buffer size must be chosen to be able to handle
the maximum possible packet length. The buffer pool uses a queue to
store pointers to free buffer elements. First of all, this gives a very
quick method to get the next free element since the dequeue is an O(1)
operation. Furthermore, since the queue is a protected operating system
primitive, it can be accessed from both task-context and
interrupt-context. The <span class="title-ref">csp\_buffer\_get()</span>
version is for task-context and
<span class="title-ref">csp\_buffer\_get\_isr()</span> is for
interrupt-context. Using fixed size buffer elements that are
preallocated is again a question of speed and safety.

Definition of a buffer element \`csp\_packet\_t\`:

``` c
/**
   CSP Packet.

   This structure is constructed to fit with all interface and protocols to prevent the
   need to copy data (zero copy).

   @note In most cases a CSP packet cannot be reused in case of send failure, because the
   lower layers may add additional data causing increased length (e.g. CRC32), convert
   the CSP id to different endian (e.g. I2C), etc.
*/
typedef struct {
        uint32_t rdp_quarantine;        // EACK quarantine period
        uint32_t timestamp_tx;          // Time the message was sent
        uint32_t timestamp_rx;          // Time the message was received

        uint16_t length;                        // Data length
        csp_id_t id;                            // CSP id (unpacked version CPU readable)

        uint8_t * frame_begin;
        uint16_t frame_length;

        /* Additional header bytes, to prepend packed data before transmission
         * This must be minimum 6 bytes to accomodate CSP 2.0. But some implementations
         * require much more scratch working area for encryption for example.
         *
         * Ultimately after csp_id_pack() this area will be filled with the CSP header
         */

        uint8_t header[CSP_PACKET_PADDING_BYTES];

        /**
         * Data part of packet.
         * When using the csp_buffer API, the size of the data part is set by
         * csp_buffer_init(), and can later be accessed by csp_buffer_data_size()
         */
        union {
                /** Access data as uint8_t. */
                uint8_t data[0];
                /** Access data as uint16_t */
                uint16_t data16[0];
                /** Access data as uint32_t */
                uint32_t data32[0];
        };

} csp_packet_t;
```

A basic concept in the buffer system is called Zero-Copy. This means
that from userspace to the kernel-driver, the buffer is never copied
from one buffer to another. This is a big deal for a small
microprocessor, where a call to <span class="title-ref">memcpy()</span>
can be very expensive. This is achieved by a number of
<span class="title-ref">padding</span> bytes in the buffer, allowing for
a header to be prepended at the lower layers without copying the actual
payload. This also means that there is a strict contract between the
layers, which data can be modified and where.

The padding bytes are used by the I2C interface, where the
<span class="title-ref">csp\_packet\_t</span> will be casted to a
<span class="title-ref">csp\_i2c\_frame\_t</span>, when the interface
calls the driver Tx function \`csp\_i2c\_driver\_tx\_t\`:

``` c
/**
   I2C frame.
   This struct fits on top of a #csp_packet_t, removing the need for copying data.
*/
typedef struct i2c_frame_s {
    //! Not used  (-> csp_packet_t.padding)
    uint8_t padding[3];
    //! Cleared before Tx  (-> csp_packet_t.padding)
    uint8_t retries;
    //! Not used  (-> csp_packet_t.padding)
    uint32_t reserved;
    //! Destination address  (-> csp_packet_t.padding)
    uint8_t dest;
    //! Cleared before Tx  (-> csp_packet_t.padding)
    uint8_t len_rx;
    //! Length of \a data part  (-> csp_packet_t.length)
    uint16_t len;
    //! CSP id + data  (-> csp_packet_t.id)
    uint8_t data[0];
} csp_i2c_frame_t;
```

## Connection

CSP supports both connection-less and connection-oriented connections.
See more about protocols in `layer4`.

During initialization libcsp allocates the configured number of
connections. The required number of connections depends on the
application. Here is a list functions, that will allocate a connection
from the connection pool:

>   - client connection, call to
>     <span class="title-ref">csp\_connect()</span>
>   - server socket for listening
>     <span class="title-ref">csp\_socket()</span>
>   - server accepting an incmoing connection
>     <span class="title-ref">csp\_accept()</span>

An applications receive queue is located on the connection and is also
allocated once during initialization. The length of the queue is the
same for all queues, and specified in the configuration.

## Send

The data flow from the application to the driver, can basically be
broken down into following steps:

> 1.  if using connection-oriented communication, establish a
>     connection\> <span class="title-ref">csp\_connect()</span>,
>     <span class="title-ref">csp\_accept()</span>
> 2.  get packet from the buffer pool:
>     <span class="title-ref">csp\_buffer\_get()</span>
> 3.  add payload data to the packet
> 4.  send packet, e.g. <span class="title-ref">csp\_send()</span>,
>     <span class="title-ref">csp\_sendto()</span>
> 5.  CSP looks up the destination route, using the routing table, and
>     calls <span class="title-ref">nexthop()</span> on the resolved
>     interface.
> 6.  The interface (in this case the CAN interface), splits the packet
>     into a number of CAN frames (8 bytes) and forwards them to the
>     driver.

## Receive

The data flow from the driver to the application, can basically be
broken down into following steps:

> 1.  the driver layer forwards the raw data frames to the interface, in
>     this case CAN frames
> 2.  the interface will aquire a free buffer (e.g.
>     <span class="title-ref">csp\_buffer\_get\_isr()</span>) for
>     assembling the CAN frames into a complete packet
> 3.  once the interface has successfully assembled a packet, the packet
>     is queued for routing - primarily to decouple the interface, e.g.
>     if the interfacec/drivers uses interrupt (ISR).
> 4.  the router picks up the packet from the incoming queue and routes
>     it on - this can either to a local destination, or another
>     interface.
> 5.  the application waits for new packets at its Rx queue, by calling
>     <span class="title-ref">csp\_read()</span> or
>     <span class="title-ref">csp\_accept</span> in case it is a server
>     socket.
> 6.  the application can now process the packet, and either send it
>     using e.g. <span class="title-ref">csp\_send()</span>, or free the
>     packet using <span class="title-ref">csp\_buffer\_free()</span>.

## Routing table

When a packet is routed, the destination address is looked up in the
routing table, which results in a
<span class="title-ref">csp\_route\_t</span> record. The record contains
the inteface (<span class="title-ref">csp\_iface\_t</span>) the packet
is to be send on, and an optional <span class="title-ref">via</span>
address. The <span class="title-ref">via</span> address is used, when
the sender cannot direcly reach the receiver on one of its connected
networks, e.g. sending a packet from the satellite to the ground - the
radio will be the <span class="title-ref">via</span> address.

CSP comes with 2 routing table implementations (selected at compile
time).

>   - static: supports a one-to-one mapping, meaning routes must be
>     configured per destination address or a single
>     <span class="title-ref">default</span> address. The
>     <span class="title-ref">default</span> address is used, in case
>     there are no routes set for the specific destination address. The
>     <span class="title-ref">static</span> routing table has the
>     fastest lookup, but requires more setup.
>   - cidr (Classless Inter-Domain Routing): supports a one-to-many
>     mapping, meaning routes can be configued for a range of
>     destianation addresses. The <span class="title-ref">cidr</span> is
>     a bit slower for lookup, but simple to setup.

Routes can be configured using text strings in the format:

> \<address\>\[/mask\] \<interface name\> \[via\]
> 
>   - address: is the destination address, the routing table will match
>     it against the CSP header destination.
>   - mask (optional): determines how many MSB bits of address are to be
>     matched. mask = 1 will only match the MSB bit, mask = 2 will match
>     2 MSB bits. Mask values different from 0 and 5, is only supported
>     by the cidr rtable.
>   - interface name: name of the interface to route the packet on
>   - via (optional) address: if different from 255, route the packet to
>     the <span class="title-ref">via</span> address, instead of the
>     address in the CSP header.

Here are some examples:

>   - "10 I2C" route destination address 10 to the "I2C" interface and
>     send it to address 10 (no <span class="title-ref">via</span>).
>   - "10 I2C 30" route destination address 10 to the "I2C" interface
>     and send it to address 30 (<span class="title-ref">via</span>).
>     The original destination address 10 is not changed in the CSP
>     header of the packet.
>   - "16/1 CAN 4" (CIDR only) route all destinations addresses 16-31 to
>     address 4 on the CAN interface.
>   - "0/0 CAN" default route, if no other matching route is found,
>     route packet onto the CAN interface.

## Interface

The interface typically implements `layer2`, and uses drivers from
`layer1` to send/receive data. The interface is a generic struct, with
no knowledge of any specific interface , protocol or driver:

``` c
/**
   CSP interface.
*/
struct csp_iface_s {
    const char *name;          //!< Name, max compare length is #CSP_IFLIST_NAME_MAX
    void * interface_data;     //!< Interface data, only known/used by the interface layer, e.g. state information.
    void * driver_data;        //!< Driver data, only known/used by the driver layer, e.g. device/channel references.
    nexthop_t nexthop;         //!< Next hop (Tx) function
    uint16_t mtu;              //!< Maximum Transmission Unit of interface
    uint8_t split_horizon_off; //!< Disable the route-loop prevention
    uint32_t tx;               //!< Successfully transmitted packets
    uint32_t rx;               //!< Successfully received packets
    uint32_t tx_error;         //!< Transmit errors (packets)
    uint32_t rx_error;         //!< Receive errors, e.g. too large message
    uint32_t drop;             //!< Dropped packets
    uint32_t autherr;          //!< Authentication errors (packets)
    uint32_t frame;            //!< Frame format errors (packets)
    uint32_t txbytes;          //!< Transmitted bytes
    uint32_t rxbytes;          //!< Received bytes
    uint32_t irq;              //!< Interrupts
    struct csp_iface_s *next;  //!< Internal, interfaces are stored in a linked list
};
```

If an interface implementation needs to store data, e.g. state
information (KISS), it can use the pointer
<span class="title-ref">interface\_data</span> to reference any data
structure needed. The driver implementation can use the pointer
<span class="title-ref">driver\_data</span> for storing data, e.g.
device number.

See function
<span class="title-ref">csp\_can\_socketcan\_open\_and\_add\_interface()</span>
in <span class="title-ref">src/drivers/can/can\_socketcan.c</span> for
an example of how to implement a CAN driver and hooking it into CSP,
using the CSP standard CAN interface.

### Send

When CSP needs to send a packet, it calls
<span class="title-ref">nexthop</span> on the interface returned by
route lookup. If the interface succeeds in sending the packet, it must
free the packet. In case of failure, the packet must not be freed by the
interface. The original idea was, that the packet could be retried later
on, without having to re-create the packet again. However, the current
implementation does not yet fully support this as some interfaces
modifies header (endian conversion) or data (adding CRC32).

### Receive

When receiving data, the driver calls into the interface with the
received data, e.g. <span class="title-ref">csp\_can\_rx()</span>. The
interface will convert/copy the data into a packet (e.g. by assembling
all CAN frames). Once a complete packet is received, the packet is
queued for later CSP processing, by calling
<span class="title-ref">csp\_qfifo\_write()</span>.
