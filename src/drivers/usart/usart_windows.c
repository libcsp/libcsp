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
                             FILE_FLAG_OVERLAPPED,
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
    DWORD dwErrors;
    COMSTAT comStat;
    OVERLAPPED osReader = {0};
    SetCommMask(ctx->fd, EV_RXCHAR);

    // Create the overlapped event. Must be closed before exiting to avoid a handle leak.
    osReader.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    while (ctx->isListening) {
        BOOL fWaitingOnRead = FALSE;

        if (osReader.hEvent == NULL) {
            csp_log_error("Failed to create overlapped read event\n");
            exit(1);
        }

        if (!fWaitingOnRead) {
            // Issue read operation.
            if (!ReadFile(ctx->fd, cbuf, 100, &bytesRead, &osReader)) {
                if (GetLastError() != ERROR_IO_PENDING) {
                        csp_log_error("ReadFile error\n");
                        ClearCommError(ctx->fd, &dwErrors, &comStat);
                } else {
                    fWaitingOnRead = TRUE;
                }
            } else {   
                ctx->rx_callback(ctx->user_data, cbuf, bytesRead, NULL);
            }
        }
        
        if (fWaitingOnRead) {
            DWORD dwRes = WaitForSingleObject(osReader.hEvent, UINT_MAX);
            switch(dwRes) {
                // Read completed.
                case WAIT_OBJECT_0:
                    if (!GetOverlappedResult(ctx->fd, &osReader, &bytesRead, TRUE)) {
                        // Error in communications; report it.
                        csp_log_error("Error in overlapped result\n");
                        ClearCommError(ctx->fd, &dwErrors, &comStat);
                    } else {
                        // Read completed successfully.
                        ctx->rx_callback(ctx->user_data, cbuf, bytesRead, NULL);
                    }
                    //  Reset flag so that another opertion can be issued.
                    fWaitingOnRead = FALSE;
                    break;

                case WAIT_TIMEOUT:
                    csp_log_error("Wait timed out\n");
                    // Operation isn't complete yet. fWaitingOnRead flag isn't
                    // changed since I'll loop back around, and I don't want
                    // to issue another read until the first one finishes.
                    //
                    // This is a good time to do some background work.
                    break;                       

                default:
                    csp_log_error("Error in waiting for single object\n");
                    exit(1);
                    // Error in the WaitForSingleObject; abort.
                    // This indicates a problem with the OVERLAPPED structure's
                    // event handle.
                    break;
            }
        }
    }
    CloseHandle(osReader.hEvent);
    return 0;
}

int csp_usart_write(csp_usart_fd_t fd, const void * data, size_t data_length) {
    OVERLAPPED osWrite = {0};
    DWORD dwWritten;
    DWORD dwRes;
    DWORD dwErrors;
    COMSTAT comStat;
    int res;

    // Create this write operation's OVERLAPPED structure's hEvent.
    osWrite.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (osWrite.hEvent == NULL) {
        csp_log_error("Failed to create event handle\n");
        res = CSP_ERR_TX;
    }

    // Issue write.
    else if (!WriteFile(fd, data, data_length, &dwWritten, &osWrite)) {
        if (GetLastError() != ERROR_IO_PENDING) { 
            // WriteFile failed, but isn't delayed. Report error and abort.
            csp_log_error("WriteFile failed\n");
            ClearCommError(fd, &dwErrors, &comStat);
            res = CSP_ERR_TX;
        } else {
            // Write is pending.
            dwRes = WaitForSingleObject(osWrite.hEvent, INFINITE);
            switch(dwRes) {
                case WAIT_OBJECT_0:
                    if (!GetOverlappedResult(fd, &osWrite, &dwWritten, FALSE)) {
                        csp_log_error("Error in overlapped result\n");
                        ClearCommError(fd, &dwErrors, &comStat);
                        res = CSP_ERR_TX;
                    } else {
                        // Write operation completed successfully.
                        res = CSP_ERR_NONE;
                    }
                    break;
            
                default:
                    // An error has occurred in WaitForSingleObject.
                    // This usually indicates a problem with the
                    // OVERLAPPED structure's event handle.
                    csp_log_error("Error in overlapped result\n");
                    ClearCommError(fd, &dwErrors, &comStat);
                    res = CSP_ERR_TX;
                    break;
            }
        }
    } else {
        // WriteFile completed immediately.
        res = CSP_ERR_NONE;
    }

    CloseHandle(osWrite.hEvent);
    return (res == CSP_ERR_NONE) ? (int)dwWritten : res;
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

    if (return_fd) {
        *return_fd = fd;
	}

    return CSP_ERR_NONE;
}
