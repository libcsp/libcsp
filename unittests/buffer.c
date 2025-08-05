#include <check.h>
#include "../include/csp/csp.h"
#include "../include/csp/csp_id.h"

#define CSP_ID2_HEADER_SIZE 6

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

START_TEST(test_clone_frame_begin_fixed)
{
	csp_init();

	csp_packet_t *src = csp_buffer_get_always();
	ck_assert_ptr_nonnull(src);

	/* Simulate a packet with no header*/
	memcpy(src->data, "hello", 6);
	src->length = 6;
	
	/* Add header to simulate a prepared to send packet */
	csp_id_prepend(src);

	csp_packet_t *clone = csp_buffer_clone(src);
	ck_assert_ptr_nonnull(clone);

	/* Verify that the data content is identical */
	ck_assert_mem_eq(clone->frame_begin + CSP_ID2_HEADER_SIZE, src->frame_begin + CSP_ID2_HEADER_SIZE, 6);

	/* Modify source data to verify that pointer not pointing the same area */
	memcpy(src->data, "world", 6);

	/* Check that clone is unaffected by src modification */
	ck_assert_mem_ne(clone->frame_begin + CSP_ID2_HEADER_SIZE, src->frame_begin + CSP_ID2_HEADER_SIZE, src->length);
	ck_assert_mem_eq(clone->frame_begin + CSP_ID2_HEADER_SIZE, "hello", 6);

	/* Ensure that frame_begin does NOT point to the same address as the original */
	ck_assert_ptr_ne(clone->frame_begin, src->frame_begin);

	csp_buffer_free(src);
	csp_buffer_free(clone);
}
END_TEST

Suite * buffer_suite(void)
{
	Suite *s;
	TCase *tc_alloc;

	s = suite_create("Packet Buffer");

	tc_alloc = tcase_create("allocate");
	tcase_add_test(tc_alloc, test_alloc_clean_734);
	tcase_add_test(tc_alloc, test_clone_frame_begin_fixed);
	suite_add_tcase(s, tc_alloc);

	return s;
}
