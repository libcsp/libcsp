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
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <termios.h>
#include <fcntl.h>

#include <csp/csp.h>
#include <sys/time.h>

int usart_stdio_id = 0;
int fd;
usart_callback_t usart_callback = NULL;

static void *serial_rx_thread(void *vptr_args);

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
#ifndef CSP_MACOSX
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

void usart_init(struct usart_conf * conf) {

	struct termios options;
	pthread_t rx_thread;

	fd = open(conf->device, O_RDWR | O_NOCTTY | O_NONBLOCK);

	if (fd < 0) {
		printf("Failed to open %s: %s\r\n", conf->device, strerror(errno));
		return;
	}

	int brate = 0;
    switch(conf->baudrate) {
    case 4800:    brate=B4800;    break;
    case 9600:    brate=B9600;    break;
    case 19200:   brate=B19200;   break;
    case 38400:   brate=B38400;   break;
    case 57600:   brate=B57600;   break;
    case 115200:  brate=B115200;  break;
    case 230400:  brate=B230400;  break;
#ifndef CSP_MACOSX
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
      printf("Unsupported baudrate requested, defaulting to 500000, requested baudrate=%u\n", conf->baudrate);
      brate=B500000;
      break;
    }

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
	tcsetattr(fd, TCSANOW, &options);
	if (tcgetattr(fd, &options) == -1)
		perror("error setting options");
	fcntl(fd, F_SETFL, 0);

	/* Flush old transmissions */
	if (tcflush(fd, TCIOFLUSH) == -1)
		printf("Error flushing serial port - %s(%d).\n", strerror(errno), errno);

	if (pthread_create(&rx_thread, NULL, serial_rx_thread, NULL) != 0)
		return;

}

void usart_set_callback(usart_callback_t callback) {
	usart_callback = callback;
}

void usart_insert(char c, void * pxTaskWoken) {
	printf("%c", c);
}

void usart_putstr(char * buf, int len) {
	if (write(fd, buf, len) != len)
		return;
}

void usart_putc(char c) {
	if (write(fd, &c, 1) != 1)
		return;
}

char usart_getc(void) {
	char c;
	if (read(fd, &c, 1) != 1) return 0;
	return c;
}

int usart_messages_waiting(int handle) {
  struct timeval tv;
  fd_set fds;
  tv.tv_sec = 0;
  tv.tv_usec = 0;
  FD_ZERO(&fds);
  FD_SET(STDIN_FILENO, &fds);
  select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
  return (FD_ISSET(0, &fds));
}

static void *serial_rx_thread(void *vptr_args) {
	unsigned int length;
	uint8_t * cbuf = malloc(100000);

	// Receive loop
	while (1) {
		length = read(fd, cbuf, 300);
		if (length <= 0) {
			perror("Error: ");
			exit(1);
		}
		if (usart_callback)
			usart_callback(cbuf, length, NULL);
	}
	return NULL;
}
