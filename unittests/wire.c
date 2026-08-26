#include <check.h>
#include <string.h>

#include "../include/csp/csp.h"
#include "../include/csp/csp_id.h"
#include "../include/csp/interfaces/csp_if_can.h"

typedef struct {
	unsigned int frame_count;
	uint32_t first_id;
	uint8_t first_dlc;
	uint8_t first_data[8];
} can_capture_t;

static int capture_can_frame(void * driver_data, uint32_t id, const uint8_t * data, uint8_t dlc,
				 const csp_packet_t * packet) {
	can_capture_t * capture = driver_data;
	(void)packet;

	if (capture->frame_count == 0) {
		capture->first_id = id;
		capture->first_dlc = dlc;
		memcpy(capture->first_data, data, dlc);
	}
	capture->frame_count++;

	return CSP_ERR_NONE;
}

START_TEST(test_csp_v2_header_golden_vector)
{
	static const uint8_t expected_header[] = {0x80, 0x07, 0x00, 0x60, 0xf3, 0xc1};

	csp_conf.version = 2;
	csp_init();

	csp_packet_t * packet = csp_buffer_get(0);
	ck_assert_ptr_nonnull(packet);

	packet->id.pri = 2;
	packet->id.dst = 7;
	packet->id.src = 24;
	packet->id.dport = 15;
	packet->id.sport = 15;
	packet->id.flags = 1;
	csp_id_prepend(packet);

	ck_assert_int_eq(packet->frame_length, sizeof(expected_header));
	ck_assert_mem_eq(packet->frame_begin, expected_header, sizeof(expected_header));
	csp_buffer_free(packet);

	csp_id_t id = csp_id_extract(expected_header);
	ck_assert_int_eq(id.pri, 2);
	ck_assert_int_eq(id.dst, 7);
	ck_assert_int_eq(id.src, 24);
	ck_assert_int_eq(id.dport, 15);
	ck_assert_int_eq(id.sport, 15);
	ck_assert_int_eq(id.flags, 1);
}
END_TEST

START_TEST(test_cfp2_can_golden_vector)
{
	static const uint8_t payload[] = {0x11, 0x22, 0x33, 0x44, 0x55};
	static const uint8_t expected_extension[] = {0x00, 0x60, 0xf3, 0xc1};
	static const uint8_t expected_frame[] = {
		0x00, 0x60, 0xf3, 0xc1, 0x11, 0x22, 0x33, 0x44,
	};
	can_capture_t capture = {0};
	csp_can_interface_data_t ifdata = {
		.tx_func = capture_can_frame,
	};
	csp_iface_t iface = {
		.addr = 24,
		.name = "CAN-test",
		.interface_data = &ifdata,
		.driver_data = &capture,
	};

	csp_conf.version = 2;
	csp_init();
	ck_assert_int_eq(csp_can_add_interface(&iface), CSP_ERR_NONE);
	atomic_store(&ifdata.cfp_packet_counter, 1);

	csp_packet_t * packet = csp_buffer_get(0);
	ck_assert_ptr_nonnull(packet);

	packet->id.pri = 2;
	packet->id.dst = 7;
	packet->id.src = 24;
	packet->id.dport = 15;
	packet->id.sport = 15;
	packet->id.flags = 1;
	packet->length = sizeof(payload);
	memcpy(packet->data, payload, sizeof(payload));

	ck_assert_int_eq(iface.nexthop(&iface, CSP_NO_VIA_ADDRESS, packet, 0), CSP_ERR_NONE);
	ck_assert_int_eq(csp_can_remove_interface(&iface), CSP_ERR_NONE);

	ck_assert_int_eq(capture.frame_count, 2);
	ck_assert_int_eq(capture.first_id, 0x1000ec22);
	ck_assert_int_eq(capture.first_dlc, sizeof(expected_frame));
	ck_assert_mem_eq(capture.first_data, expected_extension, sizeof(expected_extension));
	ck_assert_mem_eq(capture.first_data, expected_frame, sizeof(expected_frame));
}
END_TEST

Suite * wire_suite(void) {
	Suite * s = suite_create("Wire format");
	TCase * tc = tcase_create("golden vectors");

	tcase_add_test(tc, test_csp_v2_header_golden_vector);
	tcase_add_test(tc, test_cfp2_can_golden_vector);
	suite_add_tcase(s, tc);

	return s;
}
