#include <check.h>
#include "../include/csp/csp.h"
#include "../include/csp/csp_id.h"
#include "../include/csp/crypto/csp_hmac.h"

START_TEST(test_hmac_append_no_header)
{
	uint8_t test_data[] = {0x61, 0x62, 0x63}; /* abc */
	uint8_t expected[] = {0x61, 0x62, 0x63, 0x9b, 0x4a, 0x91, 0x8f};
	csp_packet_t * packet;

	csp_init();

	packet = csp_buffer_get_always();
	memcpy(packet->data, test_data, sizeof(test_data));
	packet->length = sizeof(test_data);

	csp_hmac_append(packet, false);
	ck_assert_mem_eq(packet->data, expected, sizeof(expected));

	csp_hmac_verify(packet, false);
	ck_assert_mem_eq(packet->data, test_data, sizeof(test_data));
}
END_TEST

START_TEST(test_hmac_append_include_header)
{
	uint8_t test_data[] = {0x61, 0x62, 0x63}; /* abc */
	uint8_t expected[] = {0x61, 0x62, 0x63, 0x3c, 0xc7, 0x49, 0x8b};
	csp_packet_t * packet;

	csp_init();

	packet = csp_buffer_get_always();

	csp_id_prepend(packet);

	memcpy(packet->data, test_data, sizeof(test_data));
	packet->length += sizeof(test_data);
	packet->frame_length += sizeof(test_data);

	csp_hmac_append(packet, true);
	ck_assert_mem_eq(packet->data, expected, sizeof(expected));

	csp_hmac_verify(packet, true);
	ck_assert_mem_eq(packet->data, test_data, sizeof(test_data));
}
END_TEST

Suite * hmac_suite(void)
{
	Suite *s;
	TCase *tc_hmac;

	s = suite_create("HMAC");

	tc_hmac = tcase_create("append");
	tcase_add_test(tc_hmac, test_hmac_append_no_header);
	tcase_add_test(tc_hmac, test_hmac_append_include_header);
	suite_add_tcase(s, tc_hmac);

	return s;
}
