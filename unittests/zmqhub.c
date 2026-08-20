#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <zmq.h>

#include <csp/csp.h>
#include <csp/csp_id.h>
#include <csp/interfaces/csp_if_zmqhub.h>

#include "../src/csp_qfifo.h"
#include "../src/interfaces/csp_if_zmqhub_internal.h"

typedef struct {
	void * context;
	void * sender;
	void * receiver;
	csp_iface_t iface;
} zmqhub_test_t;

static void zmqhub_test_init(zmqhub_test_t * test, unsigned int version) {

	csp_conf.version = version;
	csp_init();

	test->iface.name = "ZMQTEST";
	test->context = zmq_ctx_new();
	ck_assert_ptr_nonnull(test->context);
	test->sender = zmq_socket(test->context, ZMQ_PAIR);
	test->receiver = zmq_socket(test->context, ZMQ_PAIR);
	ck_assert_ptr_nonnull(test->sender);
	ck_assert_ptr_nonnull(test->receiver);

	const char * endpoint = (version == 1) ? "inproc://zmqhub-v1" : "inproc://zmqhub-v2";
	ck_assert_int_eq(zmq_bind(test->sender, endpoint), 0);
	ck_assert_int_eq(zmq_connect(test->receiver, endpoint), 0);
}

static void zmqhub_test_deinit(zmqhub_test_t * test) {

	ck_assert_int_eq(zmq_close(test->receiver), 0);
	ck_assert_int_eq(zmq_close(test->sender), 0);
	ck_assert_int_eq(zmq_ctx_term(test->context), 0);
}

static void send_frame(void * sender, size_t payload_length, int flags) {

	csp_packet_t packet = {0};
	ck_assert_uint_le(payload_length, sizeof(packet.data));
	packet.id.pri = CSP_PRIO_NORM;
	packet.id.src = 1;
	packet.id.dst = 2;
	packet.id.sport = 10;
	packet.id.dport = 11;
	packet.length = payload_length;
	memset(packet.data, 0x5a, payload_length);

	csp_id_prepend_fixup_cspv1(&packet);
	csp_zmqhub_fixup_cspv1_add_dest_addr(&packet);
	ck_assert_int_eq(zmq_send(sender, packet.frame_begin, packet.frame_length, flags), packet.frame_length);
}

static void expect_packet(zmqhub_test_t * test, size_t payload_length) {

	ck_assert_int_eq(csp_zmqhub_rx(test->receiver, &test->iface), CSP_ERR_NONE);
	csp_qfifo_t input;
	ck_assert_int_eq(csp_qfifo_read(&input), CSP_ERR_NONE);
	ck_assert_ptr_eq(input.iface, &test->iface);
	ck_assert_uint_eq(input.packet->length, payload_length);
	ck_assert_uint_eq(input.packet->frame_length, payload_length + csp_id_get_header_size());
	csp_buffer_free(input.packet);
}

static void expect_rejected(zmqhub_test_t * test, const void * data, size_t data_length, uint32_t frame_errors, uint32_t rx_errors) {

	const int buffers_before = csp_buffer_remaining();
	ck_assert_int_eq(zmq_send(test->sender, data, data_length, 0), (int)data_length);
	ck_assert_int_eq(csp_zmqhub_rx(test->receiver, &test->iface), CSP_ERR_INVAL);
	ck_assert_uint_eq(test->iface.frame, frame_errors);
	ck_assert_uint_eq(test->iface.rx_error, rx_errors);
	ck_assert_int_eq(csp_buffer_remaining(), buffers_before);

	csp_qfifo_t input;
	ck_assert_int_eq(csp_qfifo_read(&input), CSP_ERR_TIMEDOUT);

	/* A malformed message must not prevent reception of the next frame. */
	send_frame(test->sender, 1, 0);
	expect_packet(test, 1);
}

static void test_valid_frames(unsigned int version) {

	zmqhub_test_t test = {0};
	zmqhub_test_init(&test, version);

	send_frame(test.sender, 0, 0);
	expect_packet(&test, 0);
	send_frame(test.sender, 16, 0);
	expect_packet(&test, 16);
	send_frame(test.sender, CSP_ZMQ_MTU, 0);
	expect_packet(&test, CSP_ZMQ_MTU);

	ck_assert_uint_eq(csp_zmqhub_max_raw_frame_length(), CSP_ZMQ_MTU + ((version == 1) ? 5 : 6));
	zmqhub_test_deinit(&test);
}

static void test_invalid_lengths(unsigned int version) {

	zmqhub_test_t test = {0};
	zmqhub_test_init(&test, version);

	const size_t first_oversized_length = csp_zmqhub_max_raw_frame_length() + 1;
	const size_t large_oversized_length = first_oversized_length + 4096;
	uint8_t * data = calloc(1, large_oversized_length);
	ck_assert_ptr_nonnull(data);
	expect_rejected(&test, data, csp_zmqhub_raw_header_size() - 1, 1, 0);
	expect_rejected(&test, data, first_oversized_length, 1, 1);
	expect_rejected(&test, data, large_oversized_length, 1, 2);
	free(data);

	zmqhub_test_deinit(&test);
}

static void test_multipart(unsigned int version) {

	zmqhub_test_t test = {0};
	zmqhub_test_init(&test, version);

	send_frame(test.sender, 1, ZMQ_SNDMORE);
	const uint8_t trailing_part[] = {0xaa, 0xbb};
	ck_assert_int_eq(zmq_send(test.sender, trailing_part, sizeof(trailing_part), 0), sizeof(trailing_part));
	ck_assert_int_eq(csp_zmqhub_rx(test.receiver, &test.iface), CSP_ERR_INVAL);
	ck_assert_uint_eq(test.iface.frame, 1);

	csp_qfifo_t input;
	ck_assert_int_eq(csp_qfifo_read(&input), CSP_ERR_TIMEDOUT);
	send_frame(test.sender, 1, 0);
	expect_packet(&test, 1);

	zmqhub_test_deinit(&test);
}

START_TEST(test_zmqhub_valid_v1)
{
	test_valid_frames(1);
}
END_TEST

START_TEST(test_zmqhub_valid_v2)
{
	test_valid_frames(2);
}
END_TEST

START_TEST(test_zmqhub_invalid_lengths_v1)
{
	test_invalid_lengths(1);
}
END_TEST

START_TEST(test_zmqhub_invalid_lengths_v2)
{
	test_invalid_lengths(2);
}
END_TEST

START_TEST(test_zmqhub_multipart_v1)
{
	test_multipart(1);
}
END_TEST

START_TEST(test_zmqhub_multipart_v2)
{
	test_multipart(2);
}
END_TEST

Suite * zmqhub_suite(void) {

	Suite * suite = suite_create("ZMQHUB");
	TCase * test_case = tcase_create("receive");
	tcase_add_test(test_case, test_zmqhub_valid_v1);
	tcase_add_test(test_case, test_zmqhub_valid_v2);
	tcase_add_test(test_case, test_zmqhub_invalid_lengths_v1);
	tcase_add_test(test_case, test_zmqhub_invalid_lengths_v2);
	tcase_add_test(test_case, test_zmqhub_multipart_v1);
	tcase_add_test(test_case, test_zmqhub_multipart_v2);
	suite_add_tcase(suite, test_case);
	return suite;
}
