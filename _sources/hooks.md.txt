# Hooks in CSP

```{contents}
:depth: 3
```

All hooks in CSP are weak linked functions. Some of them have default implementations,
and your program can build and run fine without implementing the hook yourself. Others
are relying on the user to provide the functions, and linker will fail because no
implementation was found.

Usage of any of the builtin weak functions, will require that you link CSP with
-Wl,--link-whole.

TODO: implement a static assert, to catch when the program has been linked without any
implementation of `csp_print`. Because there is no runtime check on this

## Debug

```c
void csp_print(const char * fmt, ...);
void csp_output_hook(csp_id_t* idout, csp_packet_t * packet, csp_iface_t * iface, uint16_t via, int from_me);
void csp_input_hook(csp_iface_t * iface, csp_packet_t * packet);
```

CSP contains a default implementation of `csp_print` which calls `printf`. This can be overridden by implementing
`csp_print` in your own application.

`input/output_hook` are called by the router on all incoming packets, and by the sender on all outgoing packets.
The default is to print out a message if the global `csp_dbg_print_packet == 1`

## Crypto

```c
int csp_crypto_decrypt(uint8_t * ciphertext_in, uint8_t ciphertext_len, uint8_t * msg_out);
int csp_crypto_encrypt(uint8_t * msg_begin, uint8_t msg_len, uint8_t * ciphertext_out);
```

The crypto calls in `csp_if_tun` does not have a default implementation in CSP and your
application will fail to link if you use csp_if_tun without these two functions.

## Time

```c
void csp_clock_get_time(csp_timestamp_t * time);
int csp_clock_set_time(const csp_timestamp_t * time);
```

The get and set time functions rely on `arch/<os>` and all have default implementations.
On FreeRTOS they simply return 0. On posix and zephyr they call `clock_gettime(CLOCK_REALTIME, &ts)`
to get the timestamp from the kernel.

## Callback functions in CSP

These callbacks are configured in runtime, by passing a function pointer to a setter function:

```c

typedef int (*csp_sys_reboot_t)(void);
void csp_sys_set_reboot(csp_sys_reboot_t reboot);

typedef int (*csp_sys_shutdown_t)(void);
void csp_sys_set_shutdown(csp_sys_shutdown_t shutdown);

```
