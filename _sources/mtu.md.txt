# Maximum Transfer Unit

There are two things limiting the MTU of CSP.

1.  The pre-allocated buffer pool’s allocation size
2.  The link layer protocol.

So let’s assume that you have made a protocol called KISS with a MTU
of 256. The 256 is the total amount of data that you can put into the
CSP-packet. However, you need to take the overhead of the link layer
into account. Typically this could consist of a length field and/or a
start/stop flag. So the actual frame size on the link layer would for
example be 256 bytes of data + 2 bytes sync flag + 2 bytes length field.

This requires a buffer allocation of at lest 256 + 2 + 2. However, the
CSP packet itself has some reserved bytes in the beginning of the packet
(which you can see in csp_types.h) - so the recommended buffer allocation
size is MAX MTU + 16 bytes. In this case the max MTU would be 256.

If you try to pass data which is longer than the MTU, the chance is that
you will also make a buffer overflow in the CSP buffer pool. However,
lets assume that you have two interfaces one with an MTU of 200 bytes
and another with an MTU of 100 bytes. In this case you might
successfully transfer 150 bytes over the first interface, but the packet
will be rejected once it comes to the second interface.

If you want to increase your MTU of a specific link layer, it is up to
the link layer protocol to implement its own fragmentation protocol. A
good example is CAN-bus which only allows a frame size of 8 bytes.
libcsp have a small protocol for this called the “CAN fragmentation
protocol" or CFP for short. This allows data of much larger size to be
transferred over the CAN bus.

Okay, but what if you want to transfer 1000 bytes, and the network
maximum MTU is 256? Well, since CSP does not include streaming sockets,
only packets, somebody will have to split that data up into chunks. It
might be that your application have special knowledge about the datatype
you are transmitting, and that it makes sense to split the 1000 byte
content into 10 chunks of 100 byte status messages. This, application
layer delimitation might be good if you have a situation with packet
loss, because your receiver could still make good usage of the partially
delivered chunks.

But, what if you just want 1000 bytes transmitted, and you don’t care
about the fragmentation unit, and also don’t want the hassle of writing
the fragmentation code yourself? - In this case, libcsp provides SFP
(small fragmentation protocol), designed to work on the application
layer. For this purpose you will not use `csp_send` and `csp_recv`, but
`csp_sfp_send` and `csp_sfp_recv`. This will split your data into chunks
of a certain size, enumerate them and transfer over a given connection.
If a chunk is missing the SFP client will abort the reception, because
SFP does not provide retransmission. If you wish to also have
retransmission and orderly delivery you will have to open an RDP
connection and send your SFP message to that connection.
