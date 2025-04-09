#include <csp/drivers/tcp.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>

#include <pthread.h>

#include <csp/csp.h>

#define TCP_BUFFER_SIZE 300

int sockfd;
tcp_callback_t tcp_callback = NULL;

static void * tcp_rx_thread(void * vptr_args);

int tcp_init(char * address, unsigned short port) {
	int ret;
	struct sockaddr_in serv_addr;
	struct hostent * server;

	pthread_t rx_thread;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		csp_print("Unable to open TCP socket.\n");
		return -EBADF;
	}

	server = gethostbyname(address);
	if (server == NULL) {
		csp_print("Unable to resolve IP address: %s\n", address);
		return -EFAULT;
	}

	// Zero address struct
	bzero((char *)&serv_addr, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;

	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr,
		  server->h_length);

	serv_addr.sin_port = htons(port);

	uint8_t * h = (uint8_t *)&serv_addr.sin_addr.s_addr;

	printf("Attempting to connect to %u.%u.%u.%u:%d\n", h[0], h[1], h[2],
		   h[3], ntohs(serv_addr.sin_port));
	if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) <
		0) {
		csp_print("unable to connect to %s:%d", address, port);
		return errno;
	}

	ret = pthread_create(&rx_thread, NULL, tcp_rx_thread, NULL);
	if (ret)
		return ret;

	return 0;
}

void tcp_set_callback(tcp_callback_t callback) {
	tcp_callback = callback;
}

void tcp_discard(char c, void * pxTaskWoken) {
	printf("tcp discarding: %c", c);
}

void tcp_putstr(char * buf, int len) {
	if (write(sockfd, buf, len) != len)
		return;
}

void tcp_putc(char c) {
	if (write(sockfd, &c, 1) != 1)
		return;
}

static void * tcp_rx_thread(void * vptr_args) {
	unsigned int length;
	uint8_t cbuf[300];

	// Receive loop
	while (1) {
		length = read(sockfd, cbuf, 300);
		if (length <= 0) {
			perror("Error: ");
			exit(1);
		}
		if (tcp_callback)
			tcp_callback(cbuf, length, NULL);
	}
	return NULL;
}
