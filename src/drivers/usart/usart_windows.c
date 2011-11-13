
#include <stdio.h>
#include <Windows.h>
#undef interface
#include <process.h>


#include <csp/drivers/usart_windows.h>

#define SecureZeroMemory RtlSecureZeroMemory
PVOID RtlSecureZeroMemory(PVOID, SIZE_T);

static HANDLE portHandle = INVALID_HANDLE_VALUE;
static HANDLE rxThread = INVALID_HANDLE_VALUE;
static CRITICAL_SECTION txSection;
static LONG isListening = 0;
static usart_rx_func usart_rx_callback = NULL;

static void prvSendData(uint8_t *buf, size_t bufsz);
static int prvTryOpenPort(LPCWSTR intf);
static int prvTryConfigurePort(const usart_win_conf_t*);
static int prvTrySetPortTimeouts(void);

int usart_init(const usart_win_conf_t *config) {
	if( prvTryOpenPort(config->intf) ) {
		// TODO: Print standard error message, from csp_debug
		return 1;
	}

	if( prvTryConfigurePort(config) ) {
		// TODO: Print standard error message, from csp_debug
		return 1;
	}

	if( prvTrySetPortTimeouts() ) {
		// TODO: Print standard error message, from csp_debug
		return 1;
	}	

	InitializeCriticalSection(&txSection);

	return 0;
}

static int prvTryOpenPort(LPCWSTR intf) {
	portHandle = CreateFile(
		"COM4", 
		GENERIC_READ|GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);

	if( portHandle == INVALID_HANDLE_VALUE ) {
		DWORD errorCode = GetLastError();
		if( errorCode == ERROR_FILE_NOT_FOUND )
			fprintf(stderr, "Could not open serial port, because it didn't exist!\n");
		return 1;
	}
	return 0;
}

static int prvTryConfigurePort(const usart_win_conf_t* conf) {
	DCB portSettings;
	SecureZeroMemory(&portSettings, sizeof(DCB));
	portSettings.DCBlength = sizeof(DCB);
	if(!GetCommState(portHandle, &portSettings) ) {
		fprintf(stderr, "Could not get default settings for open COM port!\n");
		return -1;
	}
	portSettings.BaudRate = conf->baudrate;
	portSettings.Parity = conf->paritysetting;
	portSettings.StopBits = conf->stopbits;
	portSettings.fParity = conf->checkparity;
	portSettings.fBinary = TRUE;
	portSettings.ByteSize = conf->databits; // Data size, both for tx and rx
	if( !SetCommState(portHandle, &portSettings) ) {
		fprintf(stderr, "Error when setting COM port settings!\n");
		return 1;
	}

	GetCommState(portHandle, &portSettings);
	//  TODO: Print settings if debug enabled
	return 0;
}

static int prvTrySetPortTimeouts(void) {
	COMMTIMEOUTS timeouts;
	SecureZeroMemory(&timeouts, sizeof(COMMTIMEOUTS));

	// Setting of timeouts. This means that an ReadFile or WriteFile could return with fewer read or written bytes, due to a timeout condition
	// Return from ReadFile and WriteFile due to timeout is not an error, and is not signalled as such
	if( !GetCommTimeouts(portHandle, &timeouts) ) {
		fprintf(stderr, "Error gettings current timeout settings\n");
		return 1;
	}

	timeouts.ReadIntervalTimeout = 5; // ms that can elapse between two chars
	timeouts.ReadTotalTimeoutMultiplier = 1; // Total timeout period. This is multiplied by the requested number of bytes to read
	timeouts.ReadTotalTimeoutConstant = 5; // This is added to the result of ReadTotalTimeoutMultiplier, to get ms elapsed before timeout for read
	timeouts.WriteTotalTimeoutMultiplier = 1;
	timeouts.WriteTotalTimeoutConstant = 5;
	// To immediately receive all data in input buffer, set ReadIntervalTimeout=MAXDWORD and both ReadTotalTimeoutMultiplier & ReadTotalTimeoutConstant to zero

	if(!SetCommTimeouts(portHandle, &timeouts)) {
		fprintf(stderr, "Error setting timeouts!");
		return 1;
	}

	return 0;
}



void usart_set_rx_callback(usart_rx_func fp) {
	if( rxThread != INVALID_HANDLE_VALUE ) {
		fprintf(stderr, "Listening thread already active. Unable to change rx callback!\n");
		return;
	}
	usart_rx_callback = fp;
}

unsigned WINAPI prvRxTask(void* params) {
	DWORD bytesRead;
	DWORD eventStatus;
	uint8_t recvBuffer[24];	
	SetCommMask(portHandle, EV_RXCHAR);

	
	while(isListening) {
		WaitCommEvent(portHandle, &eventStatus, NULL);
		if( !(eventStatus & EV_RXCHAR) ) {
			continue;
		}
		if( !ReadFile(portHandle, recvBuffer, 100, &bytesRead, NULL)) {
			fprintf(stderr, "Error receiving data! Code: %lu\n", GetLastError());
			continue;
		}
		if( usart_rx_callback != NULL )
			usart_rx_callback(recvBuffer, (size_t)bytesRead, NULL);
	}
	return 0;
}

static void prvSendData(uint8_t *buf, size_t bufsz) {
	DWORD bytesTotal = 0;
	DWORD bytesActual;
	if( !WriteFile(portHandle, buf, bufsz-bytesTotal, &bytesActual, NULL) ) {
		fprintf(stderr, "Could not write data. Code: %lu\n", GetLastError());
		return;
	}
	if( !FlushFileBuffers(portHandle) ) {
		fprintf(stderr, "Could not flush write buffer. Code: %lu\n", GetLastError());
	}
}

void usart_send(uint8_t *buf, size_t bufsz) {
	EnterCriticalSection(&txSection);
	prvSendData(buf, bufsz);
	LeaveCriticalSection(&txSection);
}

void usart_shutdown(void) {
	InterlockedExchange(&isListening, 0);
	CloseHandle(portHandle);
	portHandle = INVALID_HANDLE_VALUE;
	if( rxThread != INVALID_HANDLE_VALUE ) {
		WaitForSingleObject(rxThread, INFINITE);
		rxThread = INVALID_HANDLE_VALUE;
	}
	DeleteCriticalSection(&txSection);	
}

void usart_listen(void) {
	InterlockedExchange(&isListening, 1);
	rxThread = (HANDLE)_beginthreadex(NULL, 0, &prvRxTask, NULL, 0, NULL);
}

void usart_putstr(int handle, char* buf, size_t bufsz) {
	usart_send((uint8_t*)buf, bufsz);
}

void usart_insert(int handle, char c, void *pxTaskWoken) {
}

void usart_set_callback(int handle, usart_rx_func fp) {
	usart_set_rx_callback(fp);
}
