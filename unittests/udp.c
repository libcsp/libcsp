#include <check.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "../src/interfaces/csp_if_udp_internal.h"

/* A plain UDP socket bound to the loopback address and a free port */
static int bind_any_free_port(uint16_t * port) {

	int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	ck_assert_int_ge(sockfd, 0);
	struct sockaddr_in addr = {0};
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = 0;
	ck_assert_int_eq(bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)), 0);
	socklen_t len = sizeof(addr);
	ck_assert_int_eq(getsockname(sockfd, (struct sockaddr *)&addr, &len), 0);
	*port = ntohs(addr.sin_port);
	return sockfd;
}

/* The lowest free file descriptor: it changes if a descriptor leaks */
static int lowest_free_fd(void) {

	int fd = dup(STDIN_FILENO);
	ck_assert_int_ge(fd, 0);
	close(fd);
	return fd;
}

START_TEST(test_udp_open_socket_binds_the_local_port) {

	uint16_t port = 0;
	int blocker = bind_any_free_port(&port);
	close(blocker);

	int sockfd = csp_if_udp_open_socket(port);
	ck_assert_int_ge(sockfd, 0);

	struct sockaddr_in addr = {0};
	socklen_t len = sizeof(addr);
	ck_assert_int_eq(getsockname(sockfd, (struct sockaddr *)&addr, &len), 0);
	ck_assert_uint_eq(ntohs(addr.sin_port), port);

	/* A datagram sent to the port arrives: the socket is bound, not merely open */
	int sender = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	ck_assert_int_ge(sender, 0);
	struct sockaddr_in dest = {0};
	dest.sin_family = AF_INET;
	dest.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	dest.sin_port = htons(port);
	const char datagram[] = "csp";
	ck_assert_int_eq(sendto(sender, datagram, sizeof(datagram), 0, (struct sockaddr *)&dest, sizeof(dest)), (int)sizeof(datagram));
	char received[8] = {0};
	ck_assert_int_eq(recv(sockfd, received, sizeof(received), 0), (int)sizeof(datagram));
	ck_assert_str_eq(received, datagram);

	close(sender);
	close(sockfd);
}
END_TEST

START_TEST(test_udp_open_socket_reports_a_port_in_use) {

	uint16_t port = 0;
	int blocker = bind_any_free_port(&port);

	const int free_before = lowest_free_fd();
	ck_assert_int_eq(csp_if_udp_open_socket(port), -EADDRINUSE);
	/* The socket that could not be bound is closed */
	ck_assert_int_eq(lowest_free_fd(), free_before);

	/* Once the port is free again, the next attempt succeeds */
	close(blocker);
	int sockfd = csp_if_udp_open_socket(port);
	ck_assert_int_ge(sockfd, 0);
	close(sockfd);
}
END_TEST

Suite * udp_suite(void) {
	Suite * s = suite_create("UDP interface");
	TCase * tc = tcase_create("Socket");

	tcase_add_test(tc, test_udp_open_socket_binds_the_local_port);
	tcase_add_test(tc, test_udp_open_socket_reports_a_port_in_use);
	suite_add_tcase(s, tc);

	return s;
}
