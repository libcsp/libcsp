#include <csp/drivers/usart.h>

#include <csp/csp_debug.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <termios.h>
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
	pthread_mutex_lock(&lock);
}

void csp_usart_unlock(void * driver_data) {
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

	int brate = 0;
	switch (conf->baudrate) {
		case 4800:
			brate = B4800;
			break;
		case 9600:
			brate = B9600;
			break;
		case 19200:
			brate = B19200;
			break;
		case 38400:
			brate = B38400;
			break;
		case 57600:
			brate = B57600;
			break;
		case 115200:
			brate = B115200;
			break;
		case 230400:
			brate = B230400;
			break;
		case 460800:
			brate = B460800;
			break;
		case 500000:
			brate = B500000;
			break;
		case 576000:
			brate = B576000;
			break;
		case 921600:
			brate = B921600;
			break;
		case 1000000:
			brate = B1000000;
			break;
		case 1152000:
			brate = B1152000;
			break;
		case 1500000:
			brate = B1500000;
			break;
		case 2000000:
			brate = B2000000;
			break;
		case 2500000:
			brate = B2500000;
			break;
		case 3000000:
			brate = B3000000;
			break;
		case 3500000:
			brate = B3500000;
			break;
		case 4000000:
			brate = B4000000;
			break;
		default:
			csp_print("%s: Unsupported baudrate: %u\n", __func__, conf->baudrate);
			return CSP_ERR_INVAL;
	}

	int fd = open(conf->device, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (fd < 0) {
		csp_print("%s: failed to open device: [%s], errno: %s\n", __func__, conf->device, strerror(errno));
		return CSP_ERR_INVAL;
	}

	struct termios options;
	tcgetattr(fd, &options);
	cfsetispeed(&options, brate);
	cfsetospeed(&options, brate);
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
	/* tcsetattr() succeeds if just one attribute was changed, should read back attributes and check all has been changed */
	if (tcsetattr(fd, TCSANOW, &options) != 0) {
		csp_print("%s: Failed to set attributes on device: [%s], errno: %s\n", __func__, conf->device, strerror(errno));
		close(fd);
		return CSP_ERR_DRIVER;
	}
	fcntl(fd, F_SETFL, 0);

	/* Flush old transmissions */
	if (tcflush(fd, TCIOFLUSH) != 0) {
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
