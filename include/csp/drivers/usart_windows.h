/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2011 GomSpace ApS (http://www.gomspace.com)
Copyright (C) 2011 AAUSAT3 Project (http://aausat3.space.aau.dk)

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

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
	const char *intf;
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
