#include <stdio.h>
#include <Windows.h>
#include <process.h>

#include <csp/csp.h>
#include <csp/drivers/usart.h>

static HANDLE portHandle = INVALID_HANDLE_VALUE;
static HANDLE rxThread = INVALID_HANDLE_VALUE;
static CRITICAL_SECTION txSection;
static LONG isListening = 0;
static usart_callback_t usart_callback = NULL;

static void prvSendData(char *buf, int bufsz);
static int prvTryOpenPort(const char* intf);
static int prvTryConfigurePort(const struct usart_conf*);
static int prvTrySetPortTimeouts(void);
static const char* prvParityToStr(BYTE paritySetting);

#ifdef CSP_DEBUG
static void prvPrintError(void) {
    char *messageBuffer = NULL;
    DWORD errorCode = GetLastError();
    DWORD formatMessageRet;
    formatMessageRet = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        errorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (char*)&messageBuffer,
        0,
        NULL);

    if( !formatMessageRet ) {
        csp_log_error("FormatMessage error, code: %lu", GetLastError());
        return;
    }
    csp_log_error("%s", messageBuffer);
    LocalFree(messageBuffer);
}
#endif

#ifdef CSP_DEBUG
#define printError() prvPrintError()
#else
#define printError() do {} while(0)
#endif

static int prvTryOpenPort(const char *intf) {
    portHandle = CreateFileA(
        intf, 
        GENERIC_READ|GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);

    if( portHandle == INVALID_HANDLE_VALUE ) {
        DWORD errorCode = GetLastError();
        if( errorCode == ERROR_FILE_NOT_FOUND ) {
            csp_log_error("Could not open serial port, because it didn't exist!");
        }
        else
            csp_log_error("Failure opening serial port! Code: %lu", errorCode);
        return 1;
    }
    return 0;
}

static int prvTryConfigurePort(const struct usart_conf * conf) {
    DCB portSettings = {0};
    portSettings.DCBlength = sizeof(DCB);
    if(!GetCommState(portHandle, &portSettings) ) {
        csp_log_error("Could not get default settings for open COM port! Code: %lu", GetLastError());
        return -1;
    }
    portSettings.BaudRate = conf->baudrate;
    portSettings.Parity = conf->paritysetting;
    portSettings.StopBits = conf->stopbits;
    portSettings.fParity = conf->checkparity;
    portSettings.fBinary = TRUE;
    portSettings.ByteSize = conf->databits;
    if( !SetCommState(portHandle, &portSettings) ) {
        csp_log_error("Error when setting COM port settings! Code:%lu", GetLastError());
        return 1;
    }

    GetCommState(portHandle, &portSettings);

    csp_log_info("Port: %s, Baudrate: %lu, Data bits: %d, Stop bits: %d, Parity: %s",
            conf->device, conf->baudrate, conf->databits, conf->stopbits, prvParityToStr(conf->paritysetting));
    return 0;
}

static const char* prvParityToStr(BYTE paritySetting) {
    static const char *parityStr[] = {
        "None",
        "Odd",
        "Even",
        "N/A"
    };
    char const *resultStr = NULL;

    switch(paritySetting) {
        case NOPARITY:
            resultStr = parityStr[0];
            break;
        case ODDPARITY:
            resultStr = parityStr[1];
            break;
        case EVENPARITY:
            resultStr = parityStr[2];
            break;
        default:
            resultStr = parityStr[3];
    };
    return resultStr;
}

static int prvTrySetPortTimeouts(void) {
    COMMTIMEOUTS timeouts = {0};

    if( !GetCommTimeouts(portHandle, &timeouts) ) {
        csp_log_error("Error gettings current timeout settings");
        return 1;
    }

    timeouts.ReadIntervalTimeout = 5;
    timeouts.ReadTotalTimeoutMultiplier = 1;
    timeouts.ReadTotalTimeoutConstant = 5;
    timeouts.WriteTotalTimeoutMultiplier = 1;
    timeouts.WriteTotalTimeoutConstant = 5;

    if(!SetCommTimeouts(portHandle, &timeouts)) {
        csp_log_error("Error setting timeouts!");
        return 1;
    }

    return 0;
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
        if( !ReadFile(portHandle, recvBuffer, 24, &bytesRead, NULL)) {
            csp_log_warn("Error receiving data! Code: %lu", GetLastError());
            continue;
        }
        if( usart_callback != NULL )
            usart_callback(recvBuffer, (size_t)bytesRead, NULL);
    }
    return 0;
}

static void prvSendData(char *buf, int bufsz) {
    DWORD bytesTotal = 0;
    DWORD bytesActual;
    if( !WriteFile(portHandle, buf, bufsz-bytesTotal, &bytesActual, NULL) ) {
        csp_log_error("Could not write data. Code: %lu", GetLastError());
        return;
    }
    if( !FlushFileBuffers(portHandle) ) {
        csp_log_warn("Could not flush write buffer. Code: %lu", GetLastError());
    }
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

void usart_putstr(char* buf, int bufsz) {
    EnterCriticalSection(&txSection);
    prvSendData(buf, bufsz);
    LeaveCriticalSection(&txSection);
}

void usart_insert(char c, void *pxTaskWoken) {
    /* redirect debug output to stdout */
    printf("%c", c);
}

void usart_set_callback(usart_callback_t callback) {
    usart_callback = callback;
}

void usart_init(struct usart_conf * conf) {
    if( prvTryOpenPort(conf->device) ) {
        printError();
        return;
    }

    if( prvTryConfigurePort(conf) ) {
        printError();
        return;
    }

    if( prvTrySetPortTimeouts() ) {
        printError();
        return;
    }

    InitializeCriticalSection(&txSection);

    /* Start receiver thread */
    usart_listen();
}


