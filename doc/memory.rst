How CSP uses memory
===================

CSP has been written for small microprocessor systems. The way memory is handled is therefore a tradeof between the amount used and the code efficiency. This section tries to give some answers to what the memory is used for and how it it used. The primary memory blocks in use by CSP is:

 * Routing table
 * Ports table
 * Connection table
 * Buffer pool
 * Interface list

Tables
------
The reason for using tables for the routes, ports and connections is speed. When a new packet arrives the core of CSP needs to do a quick lookup in the connection so see if it can find an existing connection to which the packet matches. If this is not found, it will take a lookup in the ports table to see if there are any applications listening on the incoming port number. Another argument of using tables are pre-allocation. The linker will reserve an area of the memory for which the routes and connections can be stored. This avoid an expensive `malloc()` call during initilization of CSP, and practically costs zero CPU instructions. The downside of using tables are the wasted memory used by unallocated ports and connections. For the routing table the argumentation is the same, pre-allocation is better than calling `malloc()`.

Buffer Pool
-----------

The buffer handling system can be compiled for either static allocation or a one-time dynamic allocation of the main memory block. After this, the buffer system is entirely self-contained. All allocated elements are of the same size, so the buffer size must be chosen to be able to handle the maximum possible packet length. The buffer pool uses a queue to store pointers to free buffer elements. First of all, this gives a very quick method to get the next free element since the dequeue is an O(1) operation. Futhermore, since the queue is a protected operating system primitive, it can be accessed from both task-context and interrupt-context. The `csp_buffer_get` version is for task-context and `csp_buffer_get_isr` is for interrupt-context. Using fixed size buffer elements that are preallocated is again a question of speed and safety.


A basic concept of the buffer system is called Zero-Copy. This means that from userspace to the kernel-driver, the buffer is never copied from one buffer to another. This is a big deal for a small microprocessor, where a call to `memcpy()` can be very expensive. In practice when data is inserted into a packet, it is shifted a certain number of bytes in order to allow for a packet header to be prepended at the lower layers. This also means that there is a strict contract between the layers, which data can be modified and where. The buffer object is normally casted to a `csp_packet_t`, but when its given to an interface on the MAC layer it's casted to a `csp_i2c_frame_t` for example.

Interface list
--------------

The interface list is a simple single-ended linked list of references to the interface specification structures. These structures are static const and allocated by the linker. The pointer to this data is inserted into the list one time during setup of the interface. Each entry in the routing table has a direct pointer to the interface element, thereby avoiding list lookup, but the list is needed in order for the dynamic route configuration to know which interfaces are available.

