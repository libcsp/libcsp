#include <check.h>
#include <stdio.h>
#include <string.h>

#include "../include/csp/csp.h"
#include "../include/csp/interfaces/csp_if_can.h"
#include "../src/interfaces/csp_if_can_internal.h"

#define VBUS_TEST_PORT  10
#define VBUS_TEST_SPORT 42

static bool dlc_representable(uint8_t len) {
	static const uint8_t sizes[] = {12, 16, 20, 24, 32, 48, 64};

	if (len <= 8) {
		return true;
	}
	for (unsigned int i = 0; i < sizeof(sizes); i++) {
		if (len == sizes[i]) {
			return true;
		}
	}
	return false;
}

START_TEST(test_frame_size)
{
	static const uint8_t valid_max[] = {8, 12, 16, 20, 24, 32, 48, 64};

	for (unsigned int m = 0; m < sizeof(valid_max); m++) {
		const uint8_t max = valid_max[m];

		for (uint16_t bytes = 0; bytes < 2100; bytes++) {
			uint8_t size = csp_can_frame_size(bytes, max);

			ck_assert_msg(dlc_representable(size), "size %u (bytes %u, max %u) not representable", size, bytes, max);
			ck_assert_uint_le(size, max);
			if (bytes >= max) {
				ck_assert_uint_eq(size, max);
			} else {
				ck_assert_uint_le(size, bytes);
			}
			if (bytes > 0) {
				ck_assert_uint_gt(size, 0);
			}
		}
	}

	/* Classic CAN behavior is unchanged: min(bytes, 8) */
	for (uint16_t bytes = 0; bytes < 100; bytes++) {
		ck_assert_uint_eq(csp_can_frame_size(bytes, 8), (bytes > 8) ? 8 : bytes);
	}

	/* Chunking terminates for any packet length without ever padding */
	for (unsigned int m = 0; m < sizeof(valid_max); m++) {
		for (uint16_t length = 1; length <= 2048; length++) {
			uint16_t remaining = length;
			unsigned int frames = 0;

			while (remaining > 0) {
				uint8_t size = csp_can_frame_size(remaining, valid_max[m]);
				ck_assert_uint_gt(size, 0);
				remaining -= size;
				ck_assert_uint_lt(++frames, 1000);
			}
		}
	}
}
END_TEST

/* Virtual CAN bus: frames transmitted on tx_iface are delivered directly to
 * rx_iface. Frames with a non DLC representable size would be padded by a
 * real controller and are a test failure. */

static csp_iface_t tx_iface, rx_iface;
static csp_can_interface_data_t tx_ifdata, rx_ifdata;
static csp_socket_t vbus_sock;
static unsigned int frame_count;

static int vbus_tx(void * driver_data, uint32_t id, const uint8_t * data, uint8_t data_size, const csp_packet_t * packet) {
	(void)driver_data;
	(void)packet;

	ck_assert_msg(dlc_representable(data_size), "frame with %u data bytes would be padded by the controller", data_size);
	ck_assert_uint_le(data_size, tx_ifdata.max_frame_size);
	frame_count++;

	return csp_can_rx(&rx_iface, id, data, data_size, 0, NULL);
}

static void vbus_setup(uint8_t max_frame_size) {

	csp_init();

	memset(&tx_iface, 0, sizeof(tx_iface));
	memset(&tx_ifdata, 0, sizeof(tx_ifdata));
	tx_iface.name = "BUS_TX";
	tx_iface.addr = 1;
	tx_iface.netmask = 8;
	tx_iface.interface_data = &tx_ifdata;
	tx_ifdata.tx_func = vbus_tx;
	tx_ifdata.max_frame_size = max_frame_size;
	ck_assert_int_eq(csp_can_add_interface(&tx_iface), CSP_ERR_NONE);

	memset(&rx_iface, 0, sizeof(rx_iface));
	memset(&rx_ifdata, 0, sizeof(rx_ifdata));
	rx_iface.name = "BUS_RX";
	rx_iface.addr = 2;
	rx_iface.netmask = 8;
	rx_iface.interface_data = &rx_ifdata;
	rx_ifdata.tx_func = vbus_tx;
	rx_ifdata.max_frame_size = max_frame_size;
	ck_assert_int_eq(csp_can_add_interface(&rx_iface), CSP_ERR_NONE);

	memset(&vbus_sock, 0, sizeof(vbus_sock));
	vbus_sock.opts = CSP_SO_CONN_LESS;
	ck_assert_int_eq(csp_bind(&vbus_sock, VBUS_TEST_PORT), CSP_ERR_NONE);
	ck_assert_int_eq(csp_listen(&vbus_sock, 0), CSP_ERR_NONE);
}

static void vbus_roundtrip(uint16_t length) {

	csp_packet_t * packet = csp_buffer_get(0);
	ck_assert_ptr_nonnull(packet);

	for (uint16_t i = 0; i < length; i++) {
		packet->data[i] = (uint8_t)(i ^ (length >> 3));
	}
	packet->length = length;

	packet->id.pri = CSP_PRIO_NORM;
	packet->id.dst = rx_iface.addr;
	packet->id.src = tx_iface.addr;
	packet->id.dport = VBUS_TEST_PORT;
	packet->id.sport = VBUS_TEST_SPORT;
	packet->id.flags = 0;

	frame_count = 0;

	/* Transmit directly through the interface nexthop */
	ck_assert_int_eq(tx_iface.nexthop(&tx_iface, CSP_NO_VIA_ADDRESS, packet, 1), CSP_ERR_NONE);

	/* Verify the fragment count against the expected chunking */
	const uint8_t max = tx_ifdata.max_frame_size;
	unsigned int expected_frames = 1;
	uint16_t sent = (length > (uint16_t)(max - 4)) ? (uint16_t)(max - 4)
												   : (uint16_t)(csp_can_frame_size(4 + length, max) - 4);
	while (sent < length) {
		sent += csp_can_frame_size(length - sent, max);
		expected_frames++;
	}
	ck_assert_uint_eq(frame_count, expected_frames);

	/* Pump the router and receive on the bound socket */
	ck_assert_int_eq(csp_route_work(), CSP_ERR_NONE);

	csp_packet_t * received = csp_recvfrom(&vbus_sock, 100);
	ck_assert_msg(received != NULL, "packet of length %u was not delivered", length);
	ck_assert_uint_eq(received->length, length);
	ck_assert_uint_eq(received->id.src, tx_iface.addr);
	ck_assert_uint_eq(received->id.dst, rx_iface.addr);
	ck_assert_uint_eq(received->id.dport, VBUS_TEST_PORT);
	ck_assert_uint_eq(received->id.sport, VBUS_TEST_SPORT);

	for (uint16_t i = 0; i < length; i++) {
		ck_assert_msg(received->data[i] == (uint8_t)(i ^ (length >> 3)),
					  "data mismatch at %u in packet of length %u", i, length);
	}

	csp_buffer_free(received);
}

START_TEST(test_canfd_end_to_end)
{
	vbus_setup(CSP_CANFD_FRAME_SIZE);
	ck_assert_uint_eq(tx_ifdata.max_frame_size, CSP_CANFD_FRAME_SIZE);

	for (uint16_t length = 0; length <= CSP_BUFFER_SIZE; length++) {
		vbus_roundtrip(length);
	}
}
END_TEST

START_TEST(test_can_classic_end_to_end)
{
	/* Classic CAN regression, including fragment counter wrap around */
	vbus_setup(CSP_CAN_FRAME_SIZE);
	ck_assert_uint_eq(tx_ifdata.max_frame_size, 8);

	for (uint16_t length = 0; length <= CSP_BUFFER_SIZE; length++) {
		vbus_roundtrip(length);
	}
}
END_TEST

START_TEST(test_canfd_requires_version2)
{
	csp_init();
	csp_conf.version = 1;

	static csp_iface_t iface;
	static csp_can_interface_data_t ifdata;
	iface.name = "CANFD_V1";
	iface.interface_data = &ifdata;
	ifdata.tx_func = vbus_tx;
	ifdata.max_frame_size = CSP_CANFD_FRAME_SIZE;

	ck_assert_int_eq(csp_can_add_interface(&iface), CSP_ERR_INVAL);
}
END_TEST

START_TEST(test_add_interface_validates_frame_size)
{
	/* Only classic CAN (8) and full size CAN FD (64) frames are supported,
	 * and the driver must set the value explicitly */
	static const uint8_t invalid[] = {0, 1, 7, 9, 12, 16, 20, 24, 32, 48, 63, 65, 255};
	static const uint8_t valid[] = {8, 64};
	static csp_iface_t ifaces[sizeof(valid)];
	static csp_can_interface_data_t ifdatas[sizeof(valid) + sizeof(invalid)];
	static char names[sizeof(valid)][CSP_IFLIST_NAME_MAX + 1];

	csp_init();

	static csp_iface_t iface;
	iface.name = "CAN_INVAL";
	for (unsigned int i = 0; i < sizeof(invalid); i++) {
		iface.interface_data = &ifdatas[sizeof(valid) + i];
		ifdatas[sizeof(valid) + i].tx_func = vbus_tx;
		ifdatas[sizeof(valid) + i].max_frame_size = invalid[i];
		ck_assert_int_eq(csp_can_add_interface(&iface), CSP_ERR_INVAL);
	}

	for (unsigned int i = 0; i < sizeof(valid); i++) {
		snprintf(names[i], sizeof(names[i]), "CAN_%u", valid[i]);
		ifaces[i].name = names[i];
		ifaces[i].interface_data = &ifdatas[i];
		ifdatas[i].tx_func = vbus_tx;
		ifdatas[i].max_frame_size = valid[i];
		ck_assert_int_eq(csp_can_add_interface(&ifaces[i]), CSP_ERR_NONE);
	}
}
END_TEST

Suite * canfd_suite(void) {

	Suite * s = suite_create("canfd");
	TCase * tc = tcase_create("canfd");

	/* The end to end tests iterate over every packet length */
	tcase_set_timeout(tc, 30);

	tcase_add_test(tc, test_frame_size);
	tcase_add_test(tc, test_canfd_end_to_end);
	tcase_add_test(tc, test_can_classic_end_to_end);
	tcase_add_test(tc, test_canfd_requires_version2);
	tcase_add_test(tc, test_add_interface_validates_frame_size);

	suite_add_tcase(s, tc);

	return s;
}
