#include <check.h>
#include <time.h>
#include "../include/csp/csp.h"
#include "../include/csp/arch/csp_time.h"

#include "../src/csp_dedup.h"

#define CSP_DEDUP_COUNT 16

/* https://github.com/libcsp/libcsp/pull/1036
 *
 * The duplicate search compared time > timestamp + window, which fails on
 * both sides of the csp_get_ms() wrap, and it stopped one entry before the
 * oldest packet in the ring.
 *
 * The tests set the time through this csp_get_ms(), which otherwise returns
 * the real time. Implementing all of csp_time.h keeps a static libcsp from
 * pulling in src/arch/posix/csp_time.c and its clashing csp_get_ms().
 */
static bool fake_clock;
static uint32_t fake_ms;

uint32_t csp_get_ms(void) {

	if (fake_clock) {
		return fake_ms;
	}

	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint32_t)((ts.tv_sec * 1000) + (ts.tv_nsec / 1000000));
}

uint32_t csp_get_ms_isr(void) {

	return csp_get_ms();
}

uint32_t csp_get_s(void) {

	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint32_t)ts.tv_sec;
}

uint32_t csp_get_s_isr(void) {

	return csp_get_s();
}

/* Pass a packet with a one byte payload through the dedup at time ms */
static bool is_duplicate(uint32_t ms, uint8_t payload) {

	csp_packet_t * packet = csp_buffer_get(0);
	ck_assert_ptr_nonnull(packet);

	packet->id = (csp_id_t){0};
	packet->data[0] = payload;
	packet->length = 1;

	fake_ms = ms;
	bool duplicate = csp_dedup_is_duplicate(packet);
	csp_buffer_free(packet);

	return duplicate;
}

START_TEST(test_dedup_window)
{
	csp_init();
	fake_clock = true;

	/* A packet and a different one both pass */
	ck_assert_int_eq(is_duplicate(1000, 1), false);
	ck_assert_int_eq(is_duplicate(1000, 2), false);

	/* A duplicate is dropped up to and including 100 ms later */
	ck_assert_int_eq(is_duplicate(1100, 1), true);

	/* 101 ms later it is outside the window */
	ck_assert_int_eq(is_duplicate(1101, 1), false);

	fake_clock = false;
}
END_TEST

START_TEST(test_dedup_oldest_entry_1036)
{
	csp_init();
	fake_clock = true;

	/* Fill the ring, packet 0 is the oldest */
	for (int i = 0; i < CSP_DEDUP_COUNT; i++) {
		ck_assert_int_eq(is_duplicate(1000, i), false);
	}

	/* A duplicate of the oldest packet is found */
	ck_assert_int_eq(is_duplicate(1000, 0), true);

	/* Once another packet overwrites it, it is gone */
	ck_assert_int_eq(is_duplicate(1000, CSP_DEDUP_COUNT), false);
	ck_assert_int_eq(is_duplicate(1000, 0), false);

	fake_clock = false;
}
END_TEST

START_TEST(test_dedup_clock_wrap_1036)
{
	csp_init();
	fake_clock = true;

	/* Packet 2 at 10 s and packet 1 at 50 ms before the wrap */
	ck_assert_int_eq(is_duplicate(UINT32_MAX - 9999, 2), false);
	ck_assert_int_eq(is_duplicate(UINT32_MAX - 49, 1), false);

	/* A duplicate 30 ms later, still before the wrap, is found */
	ck_assert_int_eq(is_duplicate(UINT32_MAX - 19, 1), true);

	/* After the wrap, packet 2 from 10 s ago is outside the window */
	ck_assert_int_eq(is_duplicate(5, 2), false);

	fake_clock = false;
}
END_TEST

Suite * dedup_suite(void)
{
	Suite *s;
	TCase *tc_search;

	s = suite_create("Deduplication");

	tc_search = tcase_create("search");
	tcase_add_test(tc_search, test_dedup_window);
	tcase_add_test(tc_search, test_dedup_oldest_entry_1036);
	tcase_add_test(tc_search, test_dedup_clock_wrap_1036);
	suite_add_tcase(s, tc_search);

	return s;
}
