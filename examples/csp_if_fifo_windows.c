#include <Windows.h>
#include <process.h>
#include <stdio.h>
#include <ctype.h>
#include <stddef.h>
#undef interface

#include <csp/csp.h>
#include <csp/csp_interface.h>

#define PIPE_BUFSIZE 1024

#define TYPE_SERVER 1
#define TYPE_CLIENT 2
#define PORT        10
#define BUF_SIZE    250


static LPCTSTR pipeName = TEXT("\\\\.\\pipe\\CSP_Pipe");

static HANDLE pipe = INVALID_HANDLE_VALUE;

unsigned WINAPI fifo_rx(void *);
unsigned WINAPI pipe_listener(void *);

void printError(void);

int csp_fifo_tx(csp_packet_t *packet, uint32_t timeout);

csp_iface_t csp_if_fifo = {
    .name = "fifo",
    .nexthop = csp_fifo_tx,
    .mtu = BUF_SIZE,
};

int csp_fifo_tx(csp_packet_t *packet, uint32_t timeout) {
    printf("csp_fifo_tx tid: %lu\n", GetCurrentThreadId());
    DWORD expectedSent = packet->length + sizeof(uint32_t) + sizeof(uint16_t);
    DWORD actualSent;
    /* Write packet to fifo */
    if( !WriteFile(pipe, &packet->length, expectedSent, &actualSent, NULL)
            || actualSent != expectedSent ) {
        printError();
    }

    csp_buffer_free(packet);
    return CSP_ERR_NONE;
}


int main(int argc, char *argv[]) {
    int me, other, type;
    char *message = "Testing CSP";
    csp_socket_t *sock = NULL;
    csp_conn_t *conn = NULL;
    csp_packet_t *packet = NULL;

    /* Run as either server or client */
    if (argc != 2) {
        printf("usage: server <server/client>\r\n");
        return -1;
    }

    /* Set type */
    if (strcmp(argv[1], "server") == 0) {
        me = 1;
        other = 2;
        type = TYPE_SERVER;
    } else if (strcmp(argv[1], "client") == 0) {
        me = 2;
        other = 1;
        type = TYPE_CLIENT;
    } else {
        printf("Invalid type. Must be either 'server' or 'client'\r\n");
        return -1;
    }

    /* Init CSP and CSP buffer system */
    if (csp_init(me) != CSP_ERR_NONE || csp_buffer_init(10, 300) != CSP_ERR_NONE) {
        printf("Failed to init CSP\r\n");
        return -1;
    }

    if( type == TYPE_SERVER ) {
        _beginthreadex(NULL, 0, pipe_listener, NULL, 0, 0);    
    } else {
        pipe = CreateFile(
            pipeName,
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            0,
            NULL);
        if( pipe == INVALID_HANDLE_VALUE ) {
            printError();
            return -1;
        }
    }

    /* Set default route and start router */
    csp_route_set(CSP_DEFAULT_ROUTE, &csp_if_fifo, CSP_NODE_MAC);
    csp_route_start_task(0, 0);

    /* Create socket and listen for incoming connections */
    if (type == TYPE_SERVER) {
        sock = csp_socket(CSP_SO_NONE);
        csp_bind(sock, PORT);
        csp_listen(sock, 5);
    }

    /* Super loop */
    while (1) {
        if (type == TYPE_SERVER) {
            /* Process incoming packet */
            conn = csp_accept(sock, 1000);
            if (conn) {
                packet = csp_read(conn, 0);
                if (packet)
                    printf("Received: %s\r\n", packet->data);
                csp_buffer_free(packet);
                csp_close(conn);
            }
        } else {
            /* Send a new packet */
            packet = csp_buffer_get(strlen(message));
            if (packet) {
                strcpy((char *) packet->data, message);
                packet->length = strlen(message);
                
                conn = csp_connect(CSP_PRIO_NORM, other, PORT, 1000, CSP_O_NONE);
                printf("Sending: %s\r\n", message);
                if (!conn || !csp_send(conn, packet, 1000))
                    return -1;
                csp_close(conn);
                Sleep(1000);
            }
        }
    }

    return 0;
}

void printError(void) {
    LPTSTR messageBuffer = NULL;
    DWORD errorCode = GetLastError();
    DWORD formatMessageRet;
    formatMessageRet = FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        errorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&messageBuffer,
        0,
        NULL);

    if( !formatMessageRet ) {
        wprintf(L"FormatMessage error, code: %lu\n", GetLastError());
        return;
    }
    
    printf("%s\n", messageBuffer);
    LocalFree(messageBuffer);
}

unsigned WINAPI pipe_listener(void *parameters) {
    while(1) {
        HANDLE pipe =  CreateNamedPipe(
                        pipeName,
                        PIPE_ACCESS_DUPLEX,
                        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                        PIPE_UNLIMITED_INSTANCES,
                        PIPE_BUFSIZE,
                        PIPE_BUFSIZE,
                        0,
                        NULL);
        BOOL clientConnected;
        if( pipe == INVALID_HANDLE_VALUE ) {
            printf("Error creating named pipe. Code %lu\n", GetLastError());
            return -1;
        }

        // True if client connects *after* server called ConnectNamedPipe 
        // or *between* CreateNamedPipe and ConnectNamedPipe
        clientConnected = 
            ConnectNamedPipe(pipe, NULL) ? TRUE : GetLastError()==ERROR_PIPE_CONNECTED;
        printf("Client connected!\n");

        if( !clientConnected ) {
            printf("Failure while listening for clients. Code %lu\n", GetLastError());
            CloseHandle(pipe);
            return -1;
        }
        printf("Create client thread\n");
        _beginthreadex(NULL, 0, fifo_rx, (PVOID)pipe, 0, 0);    
    }

    return 0;
}

unsigned WINAPI fifo_rx(void *handle) {
    printf("fifo_rx tid: %lu\n", GetCurrentThreadId());
    HANDLE pipe = (HANDLE) handle;
    csp_packet_t *buf = csp_buffer_get(BUF_SIZE);
    DWORD bytesRead;
    BOOL readSuccess;

    while(1) {
        readSuccess = 
            ReadFile(pipe, &buf->length, BUF_SIZE, &bytesRead, NULL);
        if( !readSuccess || bytesRead == 0 ) {
            csp_buffer_free(buf);
            printError();
            break;
        }
        csp_new_packet(buf, &csp_if_fifo, NULL);
        buf = csp_buffer_get(BUF_SIZE);
    }
    printf("Closing pipe to client\n");
    CloseHandle(pipe);

    return 0;
}
