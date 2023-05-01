#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <unistd.h>

short socket_create(void) {
    printf("creating socket...\n");
    return socket(AF_INET, SOCK_STREAM, 0);
}

/**
 * @brief
 * Connects to a socket, expecting a server as the endpoint
 *
 *
 * @param hSocket  socket to connect to
 * @return int: exit code from connect
 */
int socket_connect(int hSocket, int port) {


    // TODO: Fix hardcoded port
    struct sockaddr_in remote = {0};        // all members set to zero

    // TODO: Fix hardcoded address
    remote.sin_addr.s_addr = inet_addr("127.0.0.1");
    remote.sin_family = AF_INET;		   // address family: IPV4
    remote.sin_port = htons(port); // htons: converts port to big-endian for networking
    return connect(
        hSocket, (struct sockaddr *)&remote, sizeof(struct sockaddr_in)
    );
}

/**
 * @brief
 * Sends the request data to the server as a request
 *
 * @param hSocket: Socket connected to server
 * @param request: Send data
 * @param len_request: length of the send data in bytes
 * @return int
 */
int socket_send(int hSocket, char* request, short len_request) {
    struct timeval tv;
    tv.tv_sec = 20;
    tv.tv_usec = 0;

    // if setting the socket options fails
    if(setsockopt(hSocket, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(tv)) < 0) {
        printf("timeout!\n");
        return -1;
    }
    return send(hSocket, request, len_request, 0);
}

/**
 * @brief
 * receive from server connected to the socket
 *
 * @param hSocket
 * @param response
 * @param RvcSize
 * @return int
 */
int socket_receive(int hSocket,char* response, short RvcSize)
{
    int retval = -1;

    // timeval {
    //   seconds
    //   useconds
    // }
    struct timeval tv;
    tv.tv_sec = 20;     // 20 second timeout
    tv.tv_usec = 0;

    // Enums from socket-constants.h
    if(setsockopt(hSocket, SOL_SOCKET, SO_RCVTIMEO,(char *)&tv,sizeof(tv)) < 0)
    {
        printf("timeout!\n");
        return -1;
    }

             // recv() is a blocking function, which will block
             // the thread until data is received
    retval = recv(
            hSocket,    // file descriptor of the socket
            response,   // buffer for storing the response
            RvcSize,    // maximum size of the response buffer
            0           // flags:
        );

    printf("response: %s\n", response);
    return retval;
}

int main(int argc, char *argv[])
{
    int port = 8080;
    int hSocket, read_size;
    struct sockaddr_in server;
    char send_data[100] = {0};
    char receive_data[200] = {0};

    //Create socket
    hSocket = socket_create();
    if(hSocket == -1)
    {
        printf("Could not create socket\n");
        return 1;
    }
    printf("socket %d created\n", hSocket);

    //Connect to remote server
    // returns -1 on error
    if (socket_connect(hSocket, port) < 0)
    {
        perror("connect failed.\n");
        return 1;
    }

    printf("connected with server on port %d\n", port);
    printf("Enter data: ");
    fgets(send_data, 100, stdin);

    // send data on socket hSocket from send_data buffer
    socket_send(hSocket, send_data, strlen(send_data));

    // receive data into receive_data buffer
    read_size = socket_receive(hSocket, receive_data, 200);
    printf("Server Response : %s\n\n",receive_data);

    close(hSocket);                 // closes the file descriptor for the socket
    shutdown(hSocket,SHUT_RD);      // shutdown connection on all three methods
    shutdown(hSocket,SHUT_WR);
    shutdown(hSocket,SHUT_RDWR);

    return 0;
}

