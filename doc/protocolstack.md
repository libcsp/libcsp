The Protocol Stack
==================

The CSP protocol stack includes functionality on all layers of the TCP/IP model:

Layer 1: Drivers
----------------

Lib CSP is not designed for any specific processor or hardware peripheral, but yet these drivers are required in order to work. The intention of LibCSP is not to provide CAN, I2C or UART drivers for all platforms, however some drivers has been included for some platforms. If you do not find your platform supported, it is quite simple to add a driver that conforms to the CSP interfaces. For example the I2C driver just requires three functions: `init`, `send` and `recv`. For good stability and performance interrupt driven drivers are preferred in favor of polled drivers. Where applicable also DMA usage is recommended.

------------


Layer 2: MAC-layer protocols
----------------------------

The layer 2 protocol software defines a frame-format that is suitable for the media. The interface can be easily extended with implementations for even more links. For example on the space-link, Gomspace uses the CCSDS framing format with forward error correction, scrambling and a 32-bit attached sync marker. The purpose of the MAC-layer protocol is to remove all this extra information, decode the frame correctly and deliver the arrived data to the router core.

Layer 3: Router Core
--------------------

The router core is the backbone of the CSP implementation. Not only does it take care of the basic routing function, it also holds some utility code for buffer handling, which must be used by all the drivers. The router works by looking at a 32-bit CSP header which contains the usual delivery and source address together with port numbers for the connection.

There is no routing protocol for automatic route discovery, all routing tables are pre-programmed into the subsystems. This means that the overall topology must be decided before putting sub-systems together. An example will be shown later.

The buffer handling system can be compiled for either static allocation or a one-time dynamic allocation of the main memory block. After this, the buffer system is entirely self-contained. All allocated elements are of the same size, so the buffer size must be chosen to be able to handle the maximum possible packet length. Since all elements are of the same size, the search algorithm is extremely quick and requires no hard-locking of the processor while running. It can even run from both interrupt and task context at the same time.

The routing table is implemented as a full 1-to-1 map of destination addresses and next-hop interfaces. The connection table can hold a pre-defined number of connection handles at a time, and all handles must be free’d after use, just like buffers.

The main purpose of the router is to accept incoming packets and deliver them to the right message queue. Therefore, in order to listen on a port-number on the network, a task must create a socket and call the accept() call. This will make the task block and wait for incoming traffic, just like a web-server or similar. When an incoming connection is opened, the task is woken. Depending on the task-priority, the task can even preempt another task and start execution immediately. 

More examples of how to use the router and how to listen for incoming traffic is shown later. 

Layer 4: Transport Extensions
-----------------------------

As known from the TCP/IP or the OSI model, layer 3 only takes care of packet delivery, nothing else. In the TCP/IP model, the data is handed to the user-space application after the TCP protocol has ensured the packets are correctly ordered, and there are no data missing. This is very practical when using networks like the internet where traffic might take different routes in the network and packets may be lost. However for a small satellite, the internal data-bus is very reliable and delivers information in the correct order, and the space-link is too slow for a 3-way handshake protocol.

Data packets are therefore handed to the user-space application just as-is, when they entered the node. This resembles the well known UDP (user datagram protocol). UDP uses a simple transmission model without implicit hand-shaking dialogues for guaranteeing reliability, ordering, or data integrity. Thus, UDP provides an unreliable service and datagrams may arrive out of order, appear duplicated, or go missing without notice. UDP assumes that error checking and correction is either not necessary or performed in the application, avoiding the overhead of such processing at the network interface level. Time-sensitive applications often use UDP because dropping packets is preferable to waiting for delayed packets, which may not be an option in a real-time system.

In order to control data integrity when using an unreliable protocol, extra software is needed. GomSpace provides a transport layer extension to simple UDP function called RDP (reliable datagram protocol). RDP is an implementation of the RFC 908 and RFC 1151 directly, which adds automatic retransmission and packet re-ordering on top of UDP. Another way of implementing retransmission is to take care of it on application layer, or using a combination of both application layer and transport layer (RDP), which is how the File Transfer Protocol works.
