#include <check.h>
#include <string.h>
#include "../include/csp/csp.h"
#include "../include/csp/csp_iflist.h"
#include "../include/csp/csp_rtable.h"
#include "../src/csp_io.h"

/* --- Dummy nexthops ------------------------------------------------------- */

static int dummy_a_tx_count;
static int dummy_b_tx_count;

static int dummy_nexthop_a(csp_iface_t * iface, uint16_t via, csp_packet_t * packet, int from_me) {
	(void)iface; (void)via; (void)from_me;
	dummy_a_tx_count++;
	csp_buffer_free(packet);
	return CSP_ERR_NONE;
}

static int dummy_nexthop_b(csp_iface_t * iface, uint16_t via, csp_packet_t * packet, int from_me) {
	(void)iface; (void)via; (void)from_me;
	dummy_b_tx_count++;
	csp_buffer_free(packet);
	return CSP_ERR_NONE;
}

static csp_iface_t iface_a;
static csp_iface_t iface_b;

/* --- Helpers -------------------------------------------------------------- */

static void setup(void) {
	csp_init();
	dummy_a_tx_count = 0;
	dummy_b_tx_count = 0;

	/* iface_a: addr 1, subnet 0.x (top 8 bits = network) */
	memset(&iface_a, 0, sizeof(iface_a));
	iface_a.name    = "DUMMY_A";
	iface_a.addr    = 1;
	iface_a.netmask = 8;
	iface_a.nexthop = dummy_nexthop_a;

	/* iface_b: addr 257 (0x0101), subnet 1.x */
	memset(&iface_b, 0, sizeof(iface_b));
	iface_b.name    = "DUMMY_B";
	iface_b.addr    = 257;
	iface_b.netmask = 8;
	iface_b.nexthop = dummy_nexthop_b;

	csp_iflist_add(&iface_a);
	csp_iflist_add(&iface_b);
}

static void teardown(void) {
	csp_iflist_remove(&iface_a);
	csp_iflist_remove(&iface_b);
}

static void send_to(uint16_t dst) {
	csp_packet_t * pkt = csp_buffer_get_always();
	pkt->length = 0;
	csp_id_t id;
	memset(&id, 0, sizeof(id));
	id.dst = dst;
	id.pri = CSP_PRIO_NORM;
	csp_send_direct(&id, pkt, NULL);
}

/* --- Tests ---------------------------------------------------------------- */

/* TC1: both interfaces enabled — packet routed to the correct one */
START_TEST(test_enabled_routes_packet)
{
	send_to(2);   /* dest 2 is on subnet 0.x — should reach iface_a */
	ck_assert_int_eq(dummy_a_tx_count, 1);
	ck_assert_int_eq(dummy_b_tx_count, 0);
}
END_TEST

/* TC2: disable iface_a — packet is not delivered to iface_a or any other interface */
START_TEST(test_disabled_drops_packet)
{
	csp_iflist_set_routing_enabled(&iface_a, false);
	send_to(2);
	ck_assert_int_eq(dummy_a_tx_count, 0);
	ck_assert_int_eq(dummy_b_tx_count, 0);
}
END_TEST

/* TC2b: when disabled interface is reached via routing table, drop is incremented */
START_TEST(test_disabled_via_rtable_increments_drop)
{
	/* Add a routing table entry pointing directly to iface_a */
	csp_rtable_set(2, 8, &iface_a, CSP_NO_VIA_ADDRESS);
	csp_iflist_set_routing_enabled(&iface_a, false);
	uint32_t drop_before = iface_a.drop;
	send_to(2);
	ck_assert_int_eq(dummy_a_tx_count, 0);
	ck_assert_int_gt((int)iface_a.drop, (int)drop_before);
	csp_rtable_clear();
}
END_TEST

/* TC3: re-enable iface_a — routing restored */
START_TEST(test_reenable_restores_routing)
{
	csp_iflist_set_routing_enabled(&iface_a, false);
	csp_iflist_set_routing_enabled(&iface_a, true);
	send_to(2);
	ck_assert_int_eq(dummy_a_tx_count, 1);
}
END_TEST

/* TC4: disabling one interface does not affect the other */
START_TEST(test_disable_one_does_not_affect_other)
{
	csp_iflist_set_routing_enabled(&iface_a, false);
	send_to(258);  /* dest 258 is on subnet 1.x — should reach iface_b */
	ck_assert_int_eq(dummy_b_tx_count, 1);
	ck_assert_int_eq(dummy_a_tx_count, 0);
}
END_TEST

/* TC5: NULL iface passed to setter is a no-op (must not crash) */
START_TEST(test_null_iface_noop)
{
	csp_iflist_set_routing_enabled(NULL, false);  /* must not crash */
	send_to(2);
	ck_assert_int_eq(dummy_a_tx_count, 1);
}
END_TEST

Suite * routing_enable_disable_suite(void)
{
	Suite * s = suite_create("Routing Enable/Disable");
	TCase * tc = tcase_create("csp_iflist_set_routing_enabled");
	tcase_add_checked_fixture(tc, setup, teardown);
	tcase_add_test(tc, test_enabled_routes_packet);
	tcase_add_test(tc, test_disabled_drops_packet);
	tcase_add_test(tc, test_disabled_via_rtable_increments_drop);
	tcase_add_test(tc, test_reenable_restores_routing);
	tcase_add_test(tc, test_disable_one_does_not_affect_other);
	tcase_add_test(tc, test_null_iface_noop);
	suite_add_tcase(s, tc);
	return s;
}
