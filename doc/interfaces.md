CSP Interfaces
==============

This is an example of how to implement a new layer-2 interface in CSP. The example is going to show how to create a `csp_if_fifo`, using a set of [named pipes](http://en.wikipedia.org/wiki/Named_pipe). The complete interface example code can be found in `examples/fifo.c`. For an example of a fragmenting interface, see the CAN interface in `src/interfaces/csp_if_can.c`.

CSP interfaces are declared in a `csp_iface_t` structure, which sets the interface nexthop function and name. A maximum transmission unit can also be set, which forces CSP to drop outgoing packets above a certain size. The fifo interface is defined as:

``` c
#include <csp/csp.h>
#include <csp/csp_interface.h>

csp_iface_t csp_if_fifo = {
    .name = "fifo",
    .nexthop = csp_fifo_tx,
    .mtu = BUF_SIZE,
};
```

Outgoing traffic
----------------

The nexthop function takes a pointer to a CSP packet and a timeout as parameters. All outgoing packets that are routed to the interface are passed to this function:

``` c
int csp_fifo_tx(csp_packet_t *packet, uint32_t timeout) {
    write(tx_channel, &packet->length, packet->length + sizeof(uint32_t) + sizeof(uint16_t));
    csp_buffer_free(packet);
    return 1;
}
```

In the fifo interface, we simply transmit the header, length field and data using a write to the fifo. CSP does not dictate the wire format, so other interfaces may decide to e.g. ignore the length field if the physical layer provides start/stop flags. 

_Important notice: If the transmission succeeds, the interface must free the packet and return 1. If transmission fails, the nexthop function should return 0 and not free the packet, to allow retransmissions by the caller._

Incoming traffic
----------------

The interface also needs to receive incoming packets and pass it to the CSP protocol stack. In the fifo interface, this is handled by a thread that blocks on the incoming fifo and waits for packets:

``` c
void * fifo_rx(void * parameters) {
    csp_packet_t *buf = csp_buffer_get(BUF_SIZE);
    /* Wait for packet on fifo */
    while (read(rx_channel, &buf->length, BUF_SIZE) > 0) {
        csp_new_packet(buf, &csp_if_fifo, NULL);
        buf = csp_buffer_get(BUF_SIZE);
    }
}
```

A new CSP buffer is preallocated with csp_buffer_get(). When data is received, the packet is passed to CSP using `csp_new_packet()` and a new buffer is allocated for the next packet. In addition to the received packet, `csp_new_packet()` takes two additional arguments:

``` c
void csp_new_packet(csp_packet_t *packet, csp_iface_t *interface, CSP_BASE_TYPE *pxTaskWoken);
```

The calling interface must be passed in `interface` to avoid routing loops. Furthermore, `pxTaskWoken` must be set to a non-NULL value if the packet is received in an interrupt service routine. If the packet is received in task context, NULL must be passed. 'pxTaskWoken' only applies to FreeRTOS systems, and POSIX system should always set the value to NULL.

`csp_new_packet` will either accept the packet or free the packet buffer, so the interface must never free the packet after passing it to CSP.

Initialization
--------------

In order to initialize the interface, and make it available to the router, use the following function found in `csp/csp_interface.h`:

``` c
csp_route_add_if(&csp_if_fifo);
```

This actually happens automatically if you try to call `csp_route_add()` with an interface that is inknown to the router. This may however be removed in the future, in order to ensure that all interfaces are initialised before configuring the routing table. The reason is, that some products released in the future may ship with an empty routing table, which is then configured by a routing protocol rather than a static configuration.

In order to setup a manual static route, use the follwing example where the default route is set to the fifo interface:

``` c
csp_route_set(CSP_DEFAULT_ROUTE, &csp_if_fifo, CSP_NODE_MAC);
```

All outgoing traffic except loopback, is now passed to the fifo interface's nexthop function. 

Building the example
--------------------

The fifo examples can be compiled with:

``` bash
% gcc csp_if_fifo.c -o csp_if_fifo -I<CSP PATH>/include -L<CSP PATH>/build -lcsp -lpthread -lrt
```

The two named pipes are created with:

``` bash
% mkfifo server_to_client client_to_server
```

