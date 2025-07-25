#include <errno.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/debug.h>
#include <sys/file.h>
#include <sys/msg.h>
#include <sys/interrupt.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/threads.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <posix/utils.h>

int main(int argc, char **argv){
    char *deviceName;

    if(argc == 2){

        deviceName = argv[1];
    }
    else if(argc == 1){
        deviceName = "/dev/uart0";
    }
    else{
        printf("%s: Enter name of uart device\n", argv[1]);
        return EXIT_FAILURE;
    }

    int uartfd = open(deviceName, O_RDWR | O_NONBLOCK);

    if(uartfd < 0){
        printf("%s: device does not exist\n", deviceName);
        return EXIT_FAILURE;
    }

    const char *msg = "Hello world!";
    ssize_t ret = write(uartfd, msg, strlen(msg));
    printf("Managed to send %ld bytes\n", ret);

    char buffer[256];
    buffer[255] = '\0';

    ssize_t len = read(uartfd, buffer, 255);
    if(len == -1){
        perror("Read failed: ");
    }

    printf("%ld: %s\n", len, buffer);
    return EXIT_SUCCESS;
}