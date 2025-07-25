#include <csp/drivers/usart.h>

#include <csp/csp_debug.h>

#include <errno.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#include <sys/file.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/threads.h>
#include <posix/utils.h>

#define CSP_PHOENIX_UART_RX_BUFFER 32
#define RX_STACK_SZ                4096

char RX_STACK[RX_STACK_SZ];

typedef struct {
	csp_usart_callback_t rx_callback;
	void * user_data;
	int fd;
	handle_t lock;
} usart_context_t;

void csp_usart_lock(void * driver_data) {
	(void)mutexLock(((usart_context_t *)driver_data)->lock);
}

void csp_usart_unlock(void * driver_data) {
	(void)mutexUnlock(((usart_context_t *)driver_data)->lock);
}

void usart_rx_thread(void * args) {
	usart_context_t * ctx = (usart_context_t *)args;
	uint8_t * buffer = (uint8_t *)malloc(CSP_PHOENIX_UART_RX_BUFFER);
	int ptr = 0;

	if (buffer == NULL) {
		printf("%s: malloc failed\n", __func__);
		(void)endthread();
	}

	while (1) {
		/* Blocking read on one byte might be inefficient but better than busy waiting  */
		char byte;
		ssize_t ret = read(ctx->fd, &byte, 1);

		/* Error occurred */
		if (ret < 0) {
			break;
		}

		if (ret > 0) {
			buffer[ptr] = byte;
			ptr = (ptr + 1) % CSP_PHOENIX_UART_RX_BUFFER;
		}

		/* We wrapped around, flush data to CSP */
		if (ptr == 0) {
			ctx->rx_callback(ctx->user_data, buffer, CSP_PHOENIX_UART_RX_BUFFER, NULL);
		}
	}

	printf("%s: Read failed\n", __func__);
	(void)endthread();
}

int csp_usart_write(int fd, const void * data, size_t len) {
	return write(fd, data, len);
}

int csp_usart_open(const csp_usart_conf_t * conf, csp_usart_callback_t rx_callback, void * user_data, csp_usart_fd_t * return_fd)
{
	if (rx_callback == NULL) {
		return CSP_ERR_INVAL;
	}

	int fd = open(conf->device, O_RDWR);

	if (fd < 0) {
		return CSP_ERR_INVAL;
	}

	struct termios t;
	cfmakeraw(&t);

	if(tcsetattr(fd, 0, &t) < 0){
        printf("%s: Failed to set termios attribute\n", __func__);
        return CSP_ERR_DRIVER;
    }

	usart_context_t * ctx = malloc(sizeof(usart_context_t));
	if (ctx == NULL) {
		printf("%s: Malloc failed\n", __func__);
	}

	ctx->rx_callback = rx_callback;
	ctx->user_data = user_data;
	ctx->fd = fd;

	beginthread(usart_rx_thread, 2, RX_STACK, RX_STACK_SZ, (void *)ctx);

	*return_fd = fd;

	return CSP_ERR_NONE;
}
