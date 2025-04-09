#ifndef TCP_H_
#define TCP_H_

#include <stdint.h>
/**
 * Initialise TCP driver with address and prot
 * @param address string with hostname or dot address
 * @param port TCP port number
 */
int tcp_init(char *address, unsigned short port);

/**
 * In order to catch incoming chars use the callback.
 * Only one callback per interface.
 * @param handle usart[0,1,2,3]
 * @param callback function pointer
 */
typedef void (*tcp_callback_t) (uint8_t *buf, int len, void *pxTaskWoken);
void tcp_set_callback(tcp_callback_t callback);

/**
 * Discard a character
 * @param c Character to discard
 */
void tcp_discard(char c, void *pxTaskWoken);

/**
 * Polling putchar
 *
 * @param c Character to transmit
 */
void tcp_putc(char c);

/**
 * Send a buffer over TCP
 *
 * @param buf Pointer to data
 * @param len Length of data
 */
void tcp_putstr(char *buf, int len);

#endif /* TCP_H_ */
