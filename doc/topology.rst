
.. _topology:

Network Topology
================

CSP uses a network oriented terminology similar to what is known from the Internet and the TCP/IP model. A CSP network can be configured for several different topologies. The most common topology is to create two segments, one for the Satellite and one for the Ground-Station. 

.. code-block:: none

     +-------+-------+-------+-------+       bus: I2C, CAN, KISS
     |       |       |       |       |
   +---+   +---+   +---+   +---+   +---+
   |OBC|   |COM|   |EPS|   |PL1|   |PL2|     Nodes 0 - 15 (Space segment)
   +---+   +---+   +---+   +---+   +---+
             ^
             |  Radio
             v
           +---+          +----+
           |TNC|          | PC |             Nodes 16 - 31 (Ground segment)
           +---+          +----+
             |               |
             +---------------+               bus: KISS, ZMQ

The address range, from 0 to 31, has been segmented into two equal size segments. This allows for easy routing in the network. All addresses starting with binary 1 is on the ground-segment, and all addresses starting with 0 is on the space segment.

The network is configured using static routes initialised at boot-up of each sub-system. This means that the basic routing table must be assigned compile-time of each subsystem. However each node supports assigning an individual route to every single node in the network and can be changed run-time. This means that the network topology can be easily reconfigured after startup.
