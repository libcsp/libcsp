# CSP tunnels

The `csp_if_tun` interface supports encapsulating and encryption of packets between two endpoints.
The inspiration comes from IPsec.

The usage is best explained using a little example. So lets start with a satellite and a ground station

0/8 satellite bus (node id 0-63)
64/8 mission control bus (node id 64-127)

Let's say we wish to create an encrypted tunned between these two. We define a new "open" network to sit
between the satellite and the ground station

128/8 open / insecure transport network (128-195)

We now place the following nodes into these networks:

Node 1: onboard security gateway
    listens to all traffic (promiscious mode)
    has a route to 64/8 via the csp_if_tun interface (encrypts outgoing traffic)
    has a route to 130 via the TUN interface (decrypts incoming traffic)
    has a route to 0/8 satellite bus (after decryption)
    csp_if_tun interface is configured with src_addr = 130 and dest_addr = 140

Node 5: onboard radio
    has a route to 128/8 via the RF interface (so only sends out encrypted traffic)
    has a route to 140 via the CAN interface (accepts encrypted traffic only)
    rejects everything else

Node 70: ground station radio
    has a route to 128/8 via the RF interface (so only sends out encrypted traffic)
    has a route to 130 via the RF interface (accepts encrypted traffic only)
    rejects everything else

Node 65: ground security gateway
    listens to all traffic (promiscious mode)
    has a route to 0/8 via the csp_if_tun interface (encrypts outgoing traffic)
    has a route to 140 via the TUN interface (decrypts incoming traffic)
    has a route to 64/8 mission control bus (after decryption)
    csp_if_tun interface is configured with src_addr = 140 and dest_addr = 130

The tunnel and encryption can also take place on the radio's themselves using the same
methodology.

In order to use the if_tun interface for encryption you must implement the following
prototypes:

```
/** Implement these, if you use csp_if_tun */
int csp_crypto_decrypt(uint8_t * ciphertext_in, uint8_t ciphertext_len, uint8_t * msg_out); // Returns -1 for failure, length if ok
int csp_crypto_encrypt(uint8_t * msg_begin, uint8_t msg_len, uint8_t * ciphertext_out); // Returns length of encrypted data
```
