#ifndef _CSP_USART_WINDOWS_H_
#define _CSP_USART_WINDOWS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <Windows.h>
#undef interface

typedef void (*usart_rx_func)(uint8_t *buffer, int bufsz, void *pxTaskWoken);

typedef struct usart_win_conf {
	LPCWSTR intf;
	DWORD baudrate;
	BYTE databits;
	BYTE stopbits; // Domain: ONESTOPBIT, ONE5STOPBITS, TWOSTOPBITS
	BYTE paritysetting; // Domain: NOPARITY, ODDPARITY, EVENPARITY
	DWORD checkparity;
} usart_win_conf_t;

int usart_init(const usart_win_conf_t *settings);
void usart_set_rx_callback(usart_rx_func);
void usart_send(uint8_t *buf, size_t bufsz);
void usart_listen(void);
void usart_shutdown(void);

void usart_putstr(int handle, char* buf, size_t bufsz);

void usart_insert(int handle, char c, void *pxTaskWoken);

void usart_set_callback(int handle, usart_rx_func fp);

#ifdef __cplusplus
}
#endif // extern "C"

#endif // _CSP_USART_WINDOWS_H_
