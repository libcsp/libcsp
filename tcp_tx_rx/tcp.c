#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <unistd.h>

short socket_create(void) {
    printf("creating socket");
    return socket(AF_INET, SOCK_STREAM, 0);
}

int socket_connect(int hSocket) {
    int ServerPort = 8080;
    struct sockaddr_in remote = {0};
    remote.sin_addr.s_addr = inet_addr("127.0.0.1");
    remote.sin_family = AF_INET;
    remote.sin_port = htons(ServerPort);
    return connect(
        hSocket, (struct sockaddr *)&remote, sizeof(struct sockaddr_in)
    );
}

int socket_send(int hSocket, char* request, short len_request) {
    struct timeval tv;
    tv.tv_sec = 20;
    tv.tv_usec = 0;

    if(setsockopt(hSocket, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(tv)) < 0) {
        printf("timeout!\n");
        return -1;
    }
    return send(hSocket, request, len_request, 0);
}

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
        printf("Time Out\n");
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
    int hSocket, read_size;
    struct sockaddr_in server;
    char SendToServer[100] = {0};
    char server_reply[200] = {0};
    //Create socket
    hSocket = socket_create();
    if(hSocket == -1)
    {
        printf("Could not create socket\n");
        return 1;
    }
    printf("Socket is created\n");
    //Connect to remote server
    if (socket_connect(hSocket) < 0)
    {
        perror("connect failed.\n");
        return 1;
    }
    printf("Sucessfully conected with server\n");
    printf("Enter the Message: ");
    fgets(SendToServer, 100, stdin);
    //Send data to the server
    socket_send(hSocket, SendToServer, strlen(SendToServer));
    //Received the data from the server
    read_size = socket_receive(hSocket, server_reply, 200);
    printf("Server Response : %s\n\n",server_reply);
    close(hSocket);
    shutdown(hSocket,0);
    shutdown(hSocket,1);
    shutdown(hSocket,2);
    return 0;
}

