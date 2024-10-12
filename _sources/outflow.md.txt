# Outgoing Packet Flow

```
csp_send()
----------
called by: user
uses conn, uses RDP
copies flags from conn to PACKET

csp_sendto()
------------
called by: user
copies flags from user to packet
copies id to packet

    csp_send_direct()
    -----------------
    called by: send, sendto, router, rdp
    perfoms outgoing routing, selects interface

        csp_send_direct_iface()
        -----------------------
        called by: csp_send_direct, bridge
        calls output hook
        copies id to packet <- This is duplicate of csp_sendto()
        calls promisc
        applies: crc, hmac
        calls nexthop
```
