#include <check.h>
#include <string.h>

#include "../include/csp/csp.h"
#include "../include/csp/interfaces/csp_if_can.h"
#include "../include/csp/interfaces/csp_if_canfd.h"

#define VBUS_TEST_PORT  10
#define VBUS_TEST_SPORT 42

/* Valid CAN FD frame sizes, largest first */
static const uint8_t dlc_sizes[] = {64, 48, 32, 24, 20, 16, 12, 8, 7, 6, 5, 4, 3, 2, 1, 0};

static bool dlc_representable(uint8_t len) {

	for (unsigned int i = 0; i < sizeof(dlc_sizes); i++) {
		if (len == dlc_sizes[i]) {
			return true;
		}
	}
	return false;
}

/* Frames needed for a packet, plus 4 header bytes in the first frame */
static unsigned int expected_canfd_frames(uint16_t length) {

	unsigned int remaining = 4 + length;
	unsigned int frames = 0;

	do {
		for (unsigned int i = 0; i < sizeof(dlc_sizes); i++) {
			if (dlc_sizes[i] <= remaining) {
				remaining -= dlc_sizes[i];
				break;
			}
		}
		frames++;
	} while (remaining > 0);

	return frames;
}

typedef struct {
	unsigned int frame_count;
	uint32_t first_id;
	uint8_t first_size;
	uint8_t first_data[CSP_CANFD_FRAME_SIZE];
} can_capture_t;

static int capture_can_frame(void * driver_data, uint32_t id, const uint8_t * data, uint8_t data_size,
							 const csp_packet_t * packet) {
	can_capture_t * capture = driver_data;
	(void)packet;

	if (capture->frame_count == 0) {
		capture->first_id = id;
		capture->first_size = data_size;
		memcpy(capture->first_data, data, data_size);
	}
	capture->frame_count++;

	return CSP_ERR_NONE;
}

static void capture_send(const uint8_t * payload, uint16_t length, can_capture_t * capture) {

	csp_can_interface_data_t ifdata = {
		.tx_func = capture_can_frame,
	};
	csp_iface_t iface = {
		.addr = 24,
		.name = "CANFD-test",
		.interface_data = &ifdata,
		.driver_data = capture,
	};

	csp_conf.version = 2;
	csp_init();
	ck_assert_int_eq(csp_canfd_add_interface(&iface), CSP_ERR_NONE);
	atomic_store(&ifdata.cfp_packet_counter, 1);

	csp_packet_t * packet = csp_buffer_get(0);
	ck_assert_ptr_nonnull(packet);

	packet->id.pri = 2;
	packet->id.dst = 7;
	packet->id.src = 24;
	packet->id.dport = 15;
	packet->id.sport = 15;
	packet->id.flags = 1;
	packet->length = length;
	memcpy(packet->data, payload, length);

	ck_assert_int_eq(iface.nexthop(&iface, CSP_NO_VIA_ADDRESS, packet, 0), CSP_ERR_NONE);
	ck_assert_int_eq(csp_canfd_remove_interface(&iface), CSP_ERR_NONE);
}

START_TEST(test_canfd_golden_vector)
{
	/* Same header as the CFP2 golden vector in wire.c */
	static const uint8_t payload[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
	static const uint8_t expected_frame[] = {
		0x00, 0x60, 0xf3, 0xc1, 0x11, 0x22, 0x33, 0x44,
		0x55, 0x66, 0x77, 0x88,
	};
	can_capture_t capture = {0};

	capture_send(payload, sizeof(payload), &capture);

	/* A single 12 byte BEGIN + END frame */
	ck_assert_int_eq(capture.frame_count, 1);
	ck_assert_int_eq(capture.first_id, 0x1000ec23);
	ck_assert_int_eq(capture.first_size, sizeof(expected_frame));
	ck_assert_mem_eq(capture.first_data, expected_frame, sizeof(expected_frame));
}
END_TEST

START_TEST(test_canfd_no_padding)
{
	/* 4 + 5 bytes is not a valid size, so the first frame is cut to 8 */
	static const uint8_t payload[] = {0x11, 0x22, 0x33, 0x44, 0x55};
	static const uint8_t expected_frame[] = {
		0x00, 0x60, 0xf3, 0xc1, 0x11, 0x22, 0x33, 0x44,
	};
	can_capture_t capture = {0};

	capture_send(payload, sizeof(payload), &capture);

	ck_assert_int_eq(capture.frame_count, 2);
	ck_assert_int_eq(capture.first_id, 0x1000ec22);
	ck_assert_int_eq(capture.first_size, sizeof(expected_frame));
	ck_assert_mem_eq(capture.first_data, expected_frame, sizeof(expected_frame));
}
END_TEST

/* Virtual bus: frames sent on tx_iface go straight to rx_iface. A frame
 * size a controller would pad fails the test. */

static csp_iface_t tx_iface, rx_iface;
static csp_can_interface_data_t tx_ifdata, rx_ifdata;
static csp_socket_t vbus_sock;
static unsigned int frame_count;

static int vbus_tx(void * driver_data, uint32_t id, const uint8_t * data, uint8_t data_size, const csp_packet_t * packet) {
	(void)driver_data;
	(void)packet;

	ck_assert_msg(dlc_representable(data_size), "frame with %u data bytes would be padded by the controller", data_size);
	frame_count++;

	return csp_canfd_rx(&rx_iface, id, data, data_size, 0, NULL);
}

static void vbus_setup(bool tx_canfd) {

	csp_init();

	tx_iface.name = "BUS_TX";
	tx_iface.addr = 1;
	tx_iface.netmask = 8;
	tx_iface.interface_data = &tx_ifdata;
	tx_ifdata.tx_func = vbus_tx;
	if (tx_canfd) {
		ck_assert_int_eq(csp_canfd_add_interface(&tx_iface), CSP_ERR_NONE);
	} else {
		ck_assert_int_eq(csp_can_add_interface(&tx_iface), CSP_ERR_NONE);
	}

	rx_iface.name = "BUS_RX";
	rx_iface.addr = 2;
	rx_iface.netmask = 8;
	rx_iface.interface_data = &rx_ifdata;
	rx_ifdata.tx_func = vbus_tx;
	ck_assert_int_eq(csp_canfd_add_interface(&rx_iface), CSP_ERR_NONE);

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
	vbus_setup(true);

	for (uint16_t length = 0; length <= CSP_BUFFER_SIZE; length++) {
		vbus_roundtrip(length);
		ck_assert_uint_eq(frame_count, expected_canfd_frames(length));
	}
}
END_TEST

START_TEST(test_canfd_small_fragments)
{
	/* 8 byte fragments from csp_if_can, including fragment counter wrap around */
	vbus_setup(false);

	for (uint16_t length = 0; length <= CSP_BUFFER_SIZE; length++) {
		vbus_roundtrip(length);
	}
}
END_TEST

START_TEST(test_canfd_rx_rejects_oversized_frame)
{
	static const uint8_t data[CSP_CANFD_FRAME_SIZE + 1] = {0};

	vbus_setup(true);

	const uint32_t id = (((uint32_t)CFP2_BEGIN_MASK) << CFP2_BEGIN_OFFSET) |
						(((uint32_t)CFP2_END_MASK) << CFP2_END_OFFSET);
	ck_assert_int_eq(csp_canfd_rx(&rx_iface, id, data, sizeof(data), 0, NULL), CSP_ERR_INVAL);
	ck_assert_uint_eq(rx_iface.frame, 1);
}
END_TEST

START_TEST(test_canfd_requires_version2)
{
	static csp_iface_t iface;
	static csp_can_interface_data_t ifdata;

	csp_conf.version = 1;
	csp_init();

	iface.name = "CANFD_V1";
	iface.interface_data = &ifdata;
	ifdata.tx_func = vbus_tx;

	ck_assert_int_eq(csp_canfd_add_interface(&iface), CSP_ERR_NOTSUP);
}
END_TEST

Suite * canfd_suite(void) {

	Suite * s = suite_create("CAN FD");
	TCase * tc = tcase_create("canfd");

	/* The end to end tests iterate over every packet length */
	tcase_set_timeout(tc, 30);

	tcase_add_test(tc, test_canfd_golden_vector);
	tcase_add_test(tc, test_canfd_no_padding);
	tcase_add_test(tc, test_canfd_end_to_end);
	tcase_add_test(tc, test_canfd_small_fragments);
	tcase_add_test(tc, test_canfd_rx_rejects_oversized_frame);
	tcase_add_test(tc, test_canfd_requires_version2);

	suite_add_tcase(s, tc);

	return s;
}
