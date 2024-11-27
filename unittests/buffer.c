#include <check.h>
#include "../include/csp/csp.h"

/* https://github.com/libcsp/libcsp/issues/734 */
START_TEST(test_alloc_clean_734)
{
	uint8_t expected[CSP_BUFFER_SIZE];

	csp_init();

    /* use all buffer and free them */
    for (unsigned int i = 0; i < CSP_BUFFER_COUNT; i++) {
        csp_packet_t * packet = csp_buffer_get_always();
        memset(packet->data, 0, sizeof(packet->data)); /* clear buffer data */
        memcpy(packet->data, "previous_data!!", i+1);  /* put some data inside */
		packet->length = i + 1;
        csp_buffer_free(packet);
    }

	memset(expected, 0, sizeof(expected));

    /* access the data of previously used buffers */
    for (unsigned int i = 0; i < CSP_BUFFER_COUNT; i++) {
        csp_packet_t * packet = csp_buffer_get_always();
		ck_assert_mem_eq(packet->data, expected, sizeof(expected));
		ck_assert_int_eq(packet->length, 0);
        csp_buffer_free(packet);
    }
}
END_TEST

Suite * buffer_suite(void)
{
	Suite *s;
	TCase *tc_alloc;

	s = suite_create("Packet Buffer");

	tc_alloc = tcase_create("allocate");
	tcase_add_test(tc_alloc, test_alloc_clean_734);
	suite_add_tcase(s, tc_alloc);

	return s;
}
