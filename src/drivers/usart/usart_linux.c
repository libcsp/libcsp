/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2012 GomSpace ApS (http://www.gomspace.com)
Copyright (C) 2012 AAUSAT3 Project (http://aausat3.space.aau.dk)

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <csp/drivers/usart.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/time.h>

#include <csp/csp.h>
#include <csp/arch/csp_malloc.h>
#include <csp/arch/csp_thread.h>

typedef struct {
	csp_usart_callback_t rx_callback;
	void * user_data;
	csp_usart_fd_t fd;
	csp_thread_handle_t rx_thread;
} usart_context_t;

static void * usart_rx_thread(void * arg) {

	usart_context_t * ctx = arg;
	const unsigned int CBUF_SIZE = 400;
	uint8_t * cbuf = malloc(CBUF_SIZE);

	// Receive loop
	while (1) {
		int length = read(ctx->fd, cbuf, CBUF_SIZE);
		if (length <= 0) {
			csp_log_error("%s: read() failed, returned: %d", __FUNCTION__, length);
			exit(1);
		}
                ctx->rx_callback(ctx->user_data, cbuf, length, NULL);
	}
	return NULL;
}

#if (0) // Unused function and no prototype in public heaaders
int getbaud(int ifd) {
	struct termios termAttr;
	int inputSpeed = -1;
	speed_t baudRate;
	tcgetattr(ifd, &termAttr);
	/* Get the input speed. */
	baudRate = cfgetispeed(&termAttr);
	switch (baudRate) {
	case B0:
		inputSpeed = 0;
		break;
	case B50:
		inputSpeed = 50;
		break;
	case B110:
		inputSpeed = 110;
		break;
	case B134:
		inputSpeed = 134;
		break;
	case B150:
		inputSpeed = 150;
		break;
	case B200:
		inputSpeed = 200;
		break;
	case B300:
		inputSpeed = 300;
		break;
	case B600:
		inputSpeed = 600;
		break;
	case B1200:
		inputSpeed = 1200;
		break;
	case B1800:
		inputSpeed = 1800;
		break;
	case B2400:
		inputSpeed = 2400;
		break;
	case B4800:
		inputSpeed = 4800;
		break;
	case B9600:
		inputSpeed = 9600;
		break;
	case B19200:
		inputSpeed = 19200;
		break;
	case B38400:
		inputSpeed = 38400;
		break;
	case B57600:
		inputSpeed = 57600;
		break;
	case B115200:
		inputSpeed = 115200;
		break;
	case B230400:
		inputSpeed = 230400;
		break;
#if (CSP_MACOSX == 0)
	case B460800:
		inputSpeed = 460800;
		break;
	case B500000:
		inputSpeed = 500000;
		break;
	case B576000:
		inputSpeed = 576000;
		break;
	case B921600:
		inputSpeed = 921600;
		break;
	case B1000000:
		inputSpeed = 1000000;
		break;
	case B1152000:
		inputSpeed = 1152000;
		break;
	case B1500000:
		inputSpeed = 1500000;
		break;
	case B2000000:
		inputSpeed = 2000000;
		break;
	case B2500000:
		inputSpeed = 2500000;
		break;
	case B3000000:
		inputSpeed = 3000000;
		break;
	case B3500000:
		inputSpeed = 3500000;
		break;
	case B4000000:
		inputSpeed = 4000000;
		break;
#endif
	}

	return inputSpeed;

}
#endif

int csp_usart_write(csp_usart_fd_t fd, const void * data, size_t data_length) {

	if (fd >= 0) {
		int res = write(fd, data, data_length);
		if (res >= 0) {
			return res;
		}
	}
	return CSP_ERR_TX; // best matching CSP error code.

}

int csp_usart_open(const csp_usart_conf_t *conf, csp_usart_callback_t rx_callback, void * user_data, csp_usart_fd_t * return_fd) {

	int brate = 0;
	switch(conf->baudrate) {
		case 4800:    brate=B4800;    break;
		case 9600:    brate=B9600;    break;
		case 19200:   brate=B19200;   break;
		case 38400:   brate=B38400;   break;
		case 57600:   brate=B57600;   break;
		case 115200:  brate=B115200;  break;
		case 230400:  brate=B230400;  break;
#if (CSP_MACOSX == 0)
		case 460800:  brate=B460800;  break;
		case 500000:  brate=B500000;  break;
		case 576000:  brate=B576000;  break;
		case 921600:  brate=B921600;  break;
		case 1000000: brate=B1000000; break;
		case 1152000: brate=B1152000; break;
		case 1500000: brate=B1500000; break;
		case 2000000: brate=B2000000; break;
		case 2500000: brate=B2500000; break;
		case 3000000: brate=B3000000; break;
		case 3500000: brate=B3500000; break;
		case 4000000: brate=B4000000; break;
#endif
		default:
			csp_log_error("%s: Unsupported baudrate: %u", __FUNCTION__, conf->baudrate);
			return CSP_ERR_INVAL;
	}

	int fd = open(conf->device, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (fd < 0) {
		csp_log_error("%s: failed to open device: [%s], errno: %s", __FUNCTION__, conf->device, strerror(errno));
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
		csp_log_error("%s: Failed to set attributes on device: [%s], errno: %s", __FUNCTION__, conf->device, strerror(errno));
		close(fd);
		return CSP_ERR_DRIVER;
	}
	fcntl(fd, F_SETFL, 0);

	/* Flush old transmissions */
	if (tcflush(fd, TCIOFLUSH) != 0) {
		csp_log_error("%s: Error flushing device: [%s], errno: %s", __FUNCTION__, conf->device, strerror(errno));
		close(fd);
		return CSP_ERR_DRIVER;
	}

	usart_context_t * ctx = calloc(1, sizeof(*ctx));
	if (ctx == NULL) {
		csp_log_error("%s: Error allocating context, device: [%s], errno: %s", __FUNCTION__, conf->device, strerror(errno));
		close(fd);
		return CSP_ERR_NOMEM;
	}
	ctx->rx_callback = rx_callback;
	ctx->user_data = user_data;
	ctx->fd = fd;

        if (rx_callback) {
		if (csp_thread_create(usart_rx_thread, "usart_rx", 0, ctx, 0, &ctx->rx_thread) != CSP_ERR_NONE) {
			csp_log_error("%s: csp_thread_create() failed to create Rx thread for device: [%s], errno: %s", __FUNCTION__, conf->device, strerror(errno));
			free(ctx);
			close(fd);
			return CSP_ERR_NOMEM;
		}
	}

        if (return_fd) {
            *return_fd = fd;
	}

	return CSP_ERR_NONE;
}
