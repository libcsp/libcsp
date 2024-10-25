#include <check.h>
#include "../include/csp/csp.h"

#define DEFAULT_TIMEOUT 1000

/* https://github.com/libcsp/libcsp/pull/707 */
START_TEST(test_queue_free_707)
{
	char item[] = "abc";

	int qlength = 10;
	int buf_size = qlength * sizeof(item);
	char buf[buf_size];

	csp_queue_handle_t qh;
	csp_static_queue_t q;

	/* zero clear */
	memset(buf, 0, buf_size);

	csp_init();

	/* create */
	qh = csp_queue_create_static(qlength, sizeof(item), buf, &q);
	ck_assert_int_eq(csp_queue_free(qh), qlength);

	/* enqueue */
	ck_assert_int_eq(csp_queue_enqueue(qh, item, DEFAULT_TIMEOUT), CSP_QUEUE_OK);
	ck_assert_int_eq(csp_queue_free(qh), qlength - 1);

}
END_TEST

Suite * queue_suite(void)
{
	Suite *s;
	TCase *tc_free;

	s = suite_create("Queue");

	tc_free = tcase_create("free");
	tcase_add_test(tc_free, test_queue_free_707);
	suite_add_tcase(s, tc_free);

	return s;
}
