/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2012 GomSpace ApS (http://www.gomspace.com)
Copyright (C) 2012 AAUSAT3 Project (http://aausat3.space.aau.dk)

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

#include <csp/drivers/usart.h>

#include <stdio.h>
#include <Windows.h>
#include <process.h>

#include <csp/csp.h>
#include <csp/arch/csp_malloc.h>
#include <csp/arch/csp_thread.h>

typedef struct {
    csp_usart_callback_t rx_callback;
    void * user_data;
    csp_usart_fd_t fd;
    HANDLE rx_thread;
    LONG isListening;
} usart_context_t;

static int openPort(const char * device, csp_usart_fd_t * return_fd) {

    *return_fd = CreateFileA(device,
                             GENERIC_READ|GENERIC_WRITE,
                             0,
                             NULL,
                             OPEN_EXISTING,
                             0,
                             NULL);
    if (*return_fd == INVALID_HANDLE_VALUE) {
        csp_log_error("Failed to open port: [%s], error: %lu", device, GetLastError());
        return CSP_ERR_INVAL;
    }

    return CSP_ERR_NONE;
}

static int configurePort(csp_usart_fd_t fd, const csp_usart_conf_t * conf) {

    DCB portSettings = {0};
    portSettings.DCBlength = sizeof(portSettings);
    if(!GetCommState(fd, &portSettings) ) {
        csp_log_error("Could not get default settings for open COM port, error: %lu", GetLastError());
        return CSP_ERR_INVAL;
    }
    portSettings.BaudRate = conf->baudrate;
    portSettings.Parity = conf->paritysetting;
    portSettings.StopBits = conf->stopbits;
    portSettings.fParity = conf->checkparity;
    portSettings.fBinary = TRUE;
    portSettings.ByteSize = conf->databits;
    if (!SetCommState(fd, &portSettings)) {
        csp_log_error("Could not configure COM port, error: %lu", GetLastError());
        return CSP_ERR_INVAL;
    }

    return CSP_ERR_NONE;
}

static int setPortTimeouts(csp_usart_fd_t fd) {

    COMMTIMEOUTS timeouts = {0};

    if (!GetCommTimeouts(fd, &timeouts)) {
        csp_log_error("Error gettings current timeout settings, error: %lu", GetLastError());
        return CSP_ERR_INVAL;
    }

    timeouts.ReadIntervalTimeout = 5;
    timeouts.ReadTotalTimeoutMultiplier = 1;
    timeouts.ReadTotalTimeoutConstant = 5;
    timeouts.WriteTotalTimeoutMultiplier = 1;
    timeouts.WriteTotalTimeoutConstant = 5;

    if(!SetCommTimeouts(fd, &timeouts)) {
        csp_log_error("Error setting timeout settings, error: %lu", GetLastError());
        return CSP_ERR_INVAL;
    }

    return CSP_ERR_NONE;
}

static unsigned WINAPI usart_rx_thread(void* params) {

    usart_context_t * ctx = params;
    DWORD eventStatus;
    uint8_t cbuf[100];
    DWORD bytesRead;
    SetCommMask(ctx->fd, EV_RXCHAR);

    while (ctx->isListening) {
        WaitCommEvent(ctx->fd, &eventStatus, NULL);
        if (eventStatus & EV_RXCHAR) {
            if (ReadFile(ctx->fd, cbuf, sizeof(cbuf), &bytesRead, NULL)) {
                ctx->rx_callback(ctx->user_data, cbuf, bytesRead, NULL);
            } else {
                csp_log_warn("Error receiving data, error: %lu", GetLastError());
            }
        }
    }
    return 0;
}

int csp_usart_write(csp_usart_fd_t fd, const void * data, size_t data_length) {

    DWORD bytesActual;
    if (!WriteFile(fd, data, data_length, &bytesActual, NULL)) {
        return CSP_ERR_TX;
    }
    if( !FlushFileBuffers(fd) ) {
        csp_log_warn("Could not flush write buffer. Code: %lu", GetLastError());
    }
    return (int) bytesActual;
}

int csp_usart_open(const csp_usart_conf_t *conf, csp_usart_callback_t rx_callback, void * user_data, csp_usart_fd_t * return_fd) {

    csp_usart_fd_t fd;
    int res = openPort(conf->device, &fd);
    if (res != CSP_ERR_NONE) {
        return res;
    }

    res = configurePort(fd, conf);
    if (res != CSP_ERR_NONE) {
        CloseHandle(fd);
        return res;
    }

    res = setPortTimeouts(fd);
    if (res) {
        CloseHandle(fd);
        return res;
    }

    usart_context_t * ctx = csp_calloc(1, sizeof(*ctx));
    if (ctx == NULL) {
        csp_log_error("%s: Error allocating context, device: [%s], errno: %s", __FUNCTION__, conf->device, strerror(errno));
        CloseHandle(fd);
        return CSP_ERR_NOMEM;
    }
    ctx->rx_callback = rx_callback;
    ctx->user_data = user_data;
    ctx->fd = fd;
    ctx->isListening = 1;

    res = csp_thread_create(usart_rx_thread, "usart_rx", 0, ctx, 0, &ctx->rx_thread);
    if (res) {
        CloseHandle(ctx->fd);
        csp_free(ctx);
        return res;
    }

    return CSP_ERR_NONE;
}
