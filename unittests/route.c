#include <check.h>
#include <string.h>
#include "../include/csp/csp.h"
#include "../include/csp/csp_cmp.h"
#include "../include/csp/interfaces/csp_if_lo.h"

#define TEST_PORT 10

static unsigned int callback_count;

static void receive_callback(csp_packet_t * packet) {

	ck_assert_int_eq(packet->data[0], 0xAA);
	callback_count++;
	csp_buffer_free(packet);
}

/* https://github.com/libcsp/libcsp/issues/764
 *
 * If csp_bind() was called without csp_listen(), the socket rx_queue
 * was never created and the router crashed with a NULL pointer
 * dereference when a packet arrived for the socket.
 *
 * Connection-less sockets have no listen step, so they accept
 * packets from csp_bind().  Connection-oriented sockets accept
 * connections from csp_listen(), and the router drops packets
 * arriving before that.
 *
 * The tests send a packet to the local address (0) so it is routed
 * through the loopback interface into the router FIFO, then run
 * csp_route_work() to process it.
 */

static void send_to_self(void) {

	csp_packet_t * packet = csp_buffer_get(0);
	ck_assert_ptr_nonnull(packet);

	packet->length = 1;
	packet->data[0] = 0xAA;

	csp_sendto(CSP_PRIO_NORM, 0, TEST_PORT, CSP_ANY, 0, packet);
}

static void send_cmp_to_self(uint8_t code, size_t size, uint32_t opts) {

	csp_packet_t * packet = csp_buffer_get(0);
	ck_assert_ptr_nonnull(packet);

	memset(packet->data, 0, size);
	struct csp_cmp_header * cmp = (struct csp_cmp_header *)packet->data;
	cmp->type = CSP_CMP_REQUEST;
	cmp->code = code;
	packet->length = size;

	csp_sendto(CSP_PRIO_NORM, 0, CSP_CMP, TEST_PORT, opts, packet);
}

START_TEST(test_bind_no_listen_conn_less_764)
{
	static csp_socket_t sock = {0};

	csp_init();

	sock.opts = CSP_SO_CONN_LESS;
	ck_assert_int_eq(csp_bind(&sock, TEST_PORT), CSP_ERR_NONE);

	send_to_self();

	csp_route_work();

	/* The packet must be delivered even without csp_listen() */
	csp_packet_t * packet = csp_recvfrom(&sock, 0);
	ck_assert_ptr_nonnull(packet);
	ck_assert_int_eq(packet->data[0], 0xAA);
	csp_buffer_free(packet);
}
END_TEST

START_TEST(test_bind_no_listen_conn_764)
{
	static csp_socket_t sock = {0};

	csp_init();

	ck_assert_int_eq(csp_bind(&sock, TEST_PORT), CSP_ERR_NONE);

	send_to_self();

	/* Must not crash, the packet must be dropped until csp_listen() */
	csp_route_work();
	ck_assert_ptr_null(csp_accept(&sock, 0));

	/* After csp_listen(), the connection must be accepted */
	ck_assert_int_eq(csp_listen(&sock, 10), CSP_ERR_NONE);

	send_to_self();
	csp_route_work();

	csp_conn_t * conn = csp_accept(&sock, 0);
	ck_assert_ptr_nonnull(conn);

	csp_packet_t * packet = csp_read(conn, 0);
	ck_assert_ptr_nonnull(packet);
	ck_assert_int_eq(packet->data[0], 0xAA);
	csp_buffer_free(packet);
	csp_close(conn);
}
END_TEST

START_TEST(test_callback_without_crc32_972)
{
	csp_init();

	callback_count = 0;
	ck_assert_int_eq(csp_bind_callback(receive_callback, TEST_PORT), CSP_ERR_NONE);

	send_to_self();
	csp_route_work();

	ck_assert_uint_eq(callback_count, 1);
}
END_TEST

START_TEST(test_sensitive_service_requires_crc32_972)
{
	csp_init();

	ck_assert_int_eq(csp_bind_callback(csp_service_handler, CSP_CMP), CSP_ERR_NONE);

	unsigned int tx = csp_if_lo.tx;
	send_cmp_to_self(CSP_CMP_IDENT, sizeof(struct csp_cmp_ident_msg), CSP_O_NONE);
	csp_route_work();

	/* A non-sensitive request does not require CRC32. */
	ck_assert_uint_eq(csp_if_lo.tx, tx + 2);
	csp_route_work();

	tx = csp_if_lo.tx;
	send_cmp_to_self(CSP_CMP_CLOCK, sizeof(struct csp_cmp_clock_msg), CSP_O_NONE);
	csp_route_work();

	/* Only the request was sent; the service discarded it. */
	ck_assert_uint_eq(csp_if_lo.tx, tx + 1);

	tx = csp_if_lo.tx;
	send_cmp_to_self(CSP_CMP_CLOCK, sizeof(struct csp_cmp_clock_msg), CSP_O_CRC32);
	csp_route_work();

	/* The protected request was handled and produced a reply. */
	ck_assert_uint_eq(csp_if_lo.tx, tx + 2);
}
END_TEST

START_TEST(test_sensitive_service_preserves_csp_v1_972)
{
	csp_conf.version = 1;
	csp_init();

	ck_assert_int_eq(csp_bind_callback(csp_service_handler, CSP_CMP), CSP_ERR_NONE);

	unsigned int tx = csp_if_lo.tx;
	send_cmp_to_self(CSP_CMP_CLOCK, sizeof(struct csp_cmp_clock_msg), CSP_O_NONE);
	csp_route_work();

	ck_assert_uint_eq(csp_if_lo.tx, tx + 2);
}
END_TEST

Suite * route_suite(void) {
	Suite *s = suite_create("route");

	TCase *tc = tcase_create("route");
	tcase_add_test(tc, test_bind_no_listen_conn_less_764);
	tcase_add_test(tc, test_bind_no_listen_conn_764);
	tcase_add_test(tc, test_callback_without_crc32_972);
	tcase_add_test(tc, test_sensitive_service_requires_crc32_972);
	tcase_add_test(tc, test_sensitive_service_preserves_csp_v1_972);

	suite_add_tcase(s, tc);

	return s;
}
