#include <csp/drivers/usart.h>

#include <csp/csp_debug.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <asm/termbits.h>
#include <fcntl.h>
#include <sys/time.h>

#include <csp/csp.h>
#include <pthread.h>

typedef struct {
	csp_usart_callback_t rx_callback;
	void * user_data;
	csp_usart_fd_t fd;
	pthread_t rx_thread;
} usart_context_t;

/* Linux is fast, so we keep it simple by having a single lock */
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void csp_usart_lock(void * driver_data) {
	(void)driver_data; /* Avoid compiler warnings about unused parameter */
	pthread_mutex_lock(&lock);
}

void csp_usart_unlock(void * driver_data) {
	(void)driver_data; /* Avoid compiler warnings about unused parameter */
	pthread_mutex_unlock(&lock);
}

static void * usart_rx_thread(void * arg) {

	usart_context_t * ctx = arg;
	const unsigned int CBUF_SIZE = 400;
	uint8_t * cbuf = malloc(CBUF_SIZE);
	if (cbuf == NULL) {
		csp_print("%s: malloc() failed, returned NULL\n", __func__);
		exit(1);
	}

	// Receive loop
	while (1) {
		int length = read(ctx->fd, cbuf, CBUF_SIZE);
		if (length <= 0) {
			csp_print("%s: read() failed, returned: %d\n", __func__, length);
			exit(1);
		}
		ctx->rx_callback(ctx->user_data, cbuf, length, NULL);
	}
	return NULL;
}

int csp_usart_write(csp_usart_fd_t fd, const void * data, size_t data_length) {

	if (fd >= 0) {
		int res = write(fd, data, data_length);
		if (res >= 0) {
			return res;
		}
	}
	return CSP_ERR_TX;  // best matching CSP error code.
}

int csp_usart_open(const csp_usart_conf_t * conf, csp_usart_callback_t rx_callback, void * user_data, csp_usart_fd_t * return_fd) {
	if (rx_callback == NULL && return_fd == NULL) {
		csp_print("%s: No rx_callback function pointer or return_fd pointer provided\n", __func__);
		return CSP_ERR_INVAL;
	}

	int fd = open(conf->device, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (fd < 0) {
		csp_print("%s: failed to open device: [%s], errno: %s\n", __func__, conf->device, strerror(errno));
		return CSP_ERR_INVAL;
	}

	struct termios2 options;
	if (ioctl(fd, TCGETS2, &options) != 0) {
		close(fd);
		return CSP_ERR_DRIVER;
	}
	options.c_cflag &= ~(CBAUD | CIBAUD);
	options.c_cflag |= BOTHER | (BOTHER << IBSHIFT);
	options.c_ispeed = conf->baudrate;
	options.c_ospeed = conf->baudrate;
	options.c_cflag |= (CLOCAL | CREAD);
	options.c_cflag &= ~PARENB;
	options.c_cflag &= ~CSTOPB;
	options.c_cflag &= ~CSIZE;
	options.c_cflag |= CS8;
	options.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN | ISIG);
	options.c_iflag &= ~(IGNBRK | BRKINT | ICRNL | INLCR | PARMRK | INPCK | ISTRIP | IXON);
	options.c_oflag &= ~(OCRNL | ONLCR | ONLRET | ONOCR | OFILL | OPOST);
	options.c_cc[VTIME] = 0;
	options.c_cc[VMIN] = 1;
	if (ioctl(fd, TCSETS2, &options) != 0) {
		csp_print("%s: Failed to set attributes on device: [%s], errno: %s\n", __func__, conf->device, strerror(errno));
		close(fd);
		return CSP_ERR_DRIVER;
	}
	fcntl(fd, F_SETFL, 0);

	/* Flush old transmissions */
	if (ioctl(fd, TCFLSH, TCIOFLUSH) != 0) {
		csp_print("%s: Error flushing device: [%s], errno: %s\n", __func__, conf->device, strerror(errno));
		close(fd);
		return CSP_ERR_DRIVER;
	}

	if (rx_callback) {
		usart_context_t * ctx = calloc(1, sizeof(*ctx));
		if (ctx == NULL) {
			csp_print("%s: Error allocating context, device: [%s], errno: %s\n", __func__, conf->device, strerror(errno));
			close(fd);
			return CSP_ERR_NOMEM;
		}
		ctx->rx_callback = rx_callback;
		ctx->user_data = user_data;
		ctx->fd = fd;
		int ret;
		pthread_attr_t attributes;

		ret = pthread_attr_init(&attributes);
		if (ret != 0) {
			free(ctx);
			close(fd);
			return CSP_ERR_NOMEM;
		}
		pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);
		ret = pthread_create(&ctx->rx_thread, &attributes, usart_rx_thread, ctx);
		if (ret != 0) {
			csp_print("%s: pthread_create() failed to create Rx thread for device: [%s], errno: %s\n", __func__, conf->device, strerror(errno));
			free(ctx);
			close(fd);
			return CSP_ERR_NOMEM;
		}
		ret = pthread_attr_destroy(&attributes);
		if (ret != 0) {
			csp_print("%s: pthread_attr_destroy() failed: %s, errno: %d\n", __func__, strerror(ret), ret);
		}
	}

	if (return_fd) {
		*return_fd = fd;
	}

	return CSP_ERR_NONE;
}
