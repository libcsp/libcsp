#include <csp/drivers/usart.h>

#include <csp/csp_debug.h>
#include <windows.h>
#include <process.h>
#include <stdlib.h>

#include <csp/csp.h>

typedef struct {
	csp_usart_callback_t rx_callback;
	void * user_data;
	csp_usart_fd_t fd;
	HANDLE rx_thread;
	LONG isListening;
} usart_context_t;

static HANDLE mutexHandle = NULL;

void csp_usart_lock(void * driver_data) {
	WaitForSingleObject(mutexHandle, 100000);
}

void csp_usart_unlock(void * driver_data) {
	ReleaseMutex(mutexHandle);
}

static int openPort(const char * device, csp_usart_fd_t * return_fd) {

	*return_fd = CreateFileA(device,
							 GENERIC_READ | GENERIC_WRITE,
							 0,
							 NULL,
							 OPEN_EXISTING,
							 0,
							 NULL);
	if (*return_fd == INVALID_HANDLE_VALUE) {
		csp_print("Failed to open port: [%s], error: %lu\n", device, GetLastError());
		return CSP_ERR_INVAL;
	}

	return CSP_ERR_NONE;
}

static int configurePort(csp_usart_fd_t fd, const csp_usart_conf_t * conf) {

	DCB portSettings = {0};
	portSettings.DCBlength = sizeof(portSettings);
	if (!GetCommState(fd, &portSettings)) {
		csp_print("Could not get default settings for open COM port, error: %lu\n", GetLastError());
		return CSP_ERR_INVAL;
	}
	portSettings.BaudRate = conf->baudrate;
	portSettings.Parity = conf->paritysetting;
	portSettings.StopBits = conf->stopbits;
	portSettings.fParity = conf->checkparity;
	portSettings.fBinary = TRUE;
	portSettings.ByteSize = conf->databits;
	if (!SetCommState(fd, &portSettings)) {
		csp_print("Could not configure COM port, error: %lu\n", GetLastError());
		return CSP_ERR_INVAL;
	}

	return CSP_ERR_NONE;
}

static int setPortTimeouts(csp_usart_fd_t fd) {

	COMMTIMEOUTS timeouts = {0};

	if (!GetCommTimeouts(fd, &timeouts)) {
		csp_print("Error gettings current timeout settings, error: %lu\n", GetLastError());
		return CSP_ERR_INVAL;
	}

	timeouts.ReadIntervalTimeout = 5;
	timeouts.ReadTotalTimeoutMultiplier = 1;
	timeouts.ReadTotalTimeoutConstant = 5;
	timeouts.WriteTotalTimeoutMultiplier = 1;
	timeouts.WriteTotalTimeoutConstant = 5;

	if (!SetCommTimeouts(fd, &timeouts)) {
		csp_print("Error setting timeout settings, error: %lu\n", GetLastError());
		return CSP_ERR_INVAL;
	}

	return CSP_ERR_NONE;
}

static unsigned WINAPI usart_rx_thread(void * params) {

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
				csp_print("Error receiving data, error: %lu\n", GetLastError());
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
	if (!FlushFileBuffers(fd)) {
		csp_print("Could not flush write buffer. Code: %lu\n", GetLastError());
	}
	return (int)bytesActual;
}

int csp_usart_open(const csp_usart_conf_t * conf, csp_usart_callback_t rx_callback, void * user_data, csp_usart_fd_t * return_fd) {

	if (mutexHandle == NULL) {
		mutexHandle = CreateMutex(NULL, FALSE, FALSE);
	}

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

	usart_context_t * ctx = calloc(1, sizeof(*ctx));
	if (ctx == NULL) {
		csp_print("%s: Error allocating context, device: [%s], errno: %s\n", __func__, conf->device, strerror(errno));
		CloseHandle(fd);
		return CSP_ERR_NOMEM;
	}
	ctx->rx_callback = rx_callback;
	ctx->user_data = user_data;
	ctx->fd = fd;
	ctx->isListening = 1;

	uintptr_t ret = _beginthreadex(NULL, 0, usart_rx_thread, NULL, 0, NULL);
	if (ret == 0) {
		CloseHandle(ctx->fd);
		free(ctx);
		return res;
	}

	if (return_fd) {
		*return_fd = fd;
	}

	return CSP_ERR_NONE;
}
