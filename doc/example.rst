Client and server example
=========================

The following example shows a simple server/client setup, where the client sends requests to the server and receives an reply.
The code snippets are from the code: `example/simple.c` and can be compiled to a executable using `./waf configure clean build --enable-examples`.

Initialization Sequence (main)
------------------------------

This code initializes the CSP system, buffers and router core. This would also be place to add whatever external interfaces the application
should use, e.g. CAN, I2C, ZMQ, KISS.
As this example uses the default loopback interface (which is automatically added) does not require any explicit initialization.

.. literalinclude:: ../examples/simple.c
   :language: c
   :start-after: /* main

Server
------

This example shows how to create a server task that listens for incoming connections. CSP should be initialized before starting this task. Note the use of `csp_service_handler()` as the default branch in the port switch case. The service handler will automatically reply to ICMP-like requests, such as pings and buffer status requests.

.. literalinclude:: ../examples/simple.c
   :language: c
   :start-after: /* Server
   :end-before: /* End

Client
------

This example shows how to allocate a packet buffer, connect to another host and send the packet. CSP should be initialized before calling this function. RDP, XTEA, HMAC and CRC checksums can be enabled per connection, by setting the connection option to a bitwise OR of any combination of `CSP_O_RDP`, `CSP_O_XTEA`, `CSP_O_HMAC` and `CSP_O_CRC`.

.. literalinclude:: ../examples/simple.c
   :language: c
   :start-after: /* Client
   :end-before: /* End
