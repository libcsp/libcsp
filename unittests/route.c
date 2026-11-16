#include <check.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <stdbool.h>

#include "../include/csp/csp.h"

#define TEST_PORT 10
#define TEST_CALLBACK_PORT 11

/* https://github.com/libcsp/libcsp/issues/764
 *
 * If csp_bind() was called without csp_listen(), the socket rx_queue
 * was never created and the router crashed with a NULL pointer
 * dereference when a packet arrived for the socket.
 *
 * Connection-less sockets have no listen step, so they accept
 * packets from csp_bind(). Connection-oriented sockets accept
 * connections from csp_listen(), and the router drops packets
 * arriving before that.
 */

static void send_to_port(uint8_t port)
{
    csp_packet_t *packet = csp_buffer_get(0);
    ck_assert_ptr_nonnull(packet);

    packet->length = 1;
    packet->data[0] = 0xAA;

    csp_sendto(CSP_PRIO_NORM, 0, port, CSP_ANY, CSP_O_CRC32, packet);
}

/* --------------------------------------------------------------------------
 * Socket binding tests
 * -------------------------------------------------------------------------- */

START_TEST(test_bind_no_listen_conn_less_764)
{
    static csp_socket_t sock = {0};

    csp_init();

    sock.opts = CSP_SO_CONN_LESS;
    ck_assert_int_eq(csp_bind(&sock, TEST_PORT), CSP_ERR_NONE);

    send_to_port(TEST_PORT);

    csp_route_work();

    /* The packet must be delivered even without csp_listen(). */
    csp_packet_t *packet = csp_recvfrom(&sock, 0);
    ck_assert_ptr_nonnull(packet);
    ck_assert_int_eq(packet->data[0], 0xAA);

    csp_buffer_free(packet);
}
END_TEST

START_TEST(test_bind_no_listen_conn_764)
{
    static csp_socket_t sock = {0};

    csp_init();

    ck_assert_int_eq(csp_bind(&sock, TEST_PORT), CSP_ERR_NONE);

    send_to_port(TEST_PORT);

    /* Must not crash; the packet must be dropped until csp_listen(). */
    csp_route_work();
    ck_assert_ptr_null(csp_accept(&sock, 0));

    /* After csp_listen(), the connection must be accepted. */
    ck_assert_int_eq(csp_listen(&sock, 10), CSP_ERR_NONE);

    send_to_port(TEST_PORT);
    csp_route_work();

    csp_conn_t *conn = csp_accept(&sock, 0);
    ck_assert_ptr_nonnull(conn);

    csp_packet_t *packet = csp_read(conn, 0);
    ck_assert_ptr_nonnull(packet);
    ck_assert_int_eq(packet->data[0], 0xAA);

    csp_buffer_free(packet);
    csp_close(conn);
}
END_TEST

/* --------------------------------------------------------------------------
 * Callback binding tests
 * -------------------------------------------------------------------------- */

static unsigned int callback_count;
static uint8_t callback_data;

static void test_callback(csp_packet_t *packet)
{
    callback_count++;
    callback_data = packet->data[0];

    csp_buffer_free(packet);
}

START_TEST(test_bind_callback)
{
    csp_init();

    callback_count = 0;
    callback_data = 0;

    ck_assert_int_eq(csp_bind_callback(test_callback, TEST_CALLBACK_PORT),
                     CSP_ERR_NONE);

    send_to_port(TEST_CALLBACK_PORT);
    csp_route_work();

    ck_assert_uint_eq(callback_count, 1);
    ck_assert_uint_eq(callback_data, 0xAA);

    ck_assert_int_eq(csp_unbind_callback(TEST_CALLBACK_PORT), CSP_ERR_NONE);
}
END_TEST

START_TEST(test_unbind_callback)
{
    csp_init();

    callback_count = 0;

    ck_assert_int_eq(csp_bind_callback(test_callback, TEST_CALLBACK_PORT),
                     CSP_ERR_NONE);

    ck_assert_int_eq(csp_unbind_callback(TEST_CALLBACK_PORT), CSP_ERR_NONE);

    send_to_port(TEST_CALLBACK_PORT);
    csp_route_work();

    ck_assert_uint_eq(callback_count, 0);
}
END_TEST

/* 
 * Concurrent callback dispatch / unbind test
 */

static pthread_mutex_t callback_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t callback_cond = PTHREAD_COND_INITIALIZER;

static bool callback_entered;
static bool callback_release;
static bool unbind_started;
static bool unbind_completed;

static int unbind_result;

static void blocking_callback(csp_packet_t *packet)
{
    pthread_mutex_lock(&callback_mutex);

    callback_entered = true;
    pthread_cond_broadcast(&callback_cond);

    while (!callback_release) {
        pthread_cond_wait(&callback_cond, &callback_mutex);
    }

    pthread_mutex_unlock(&callback_mutex);

    csp_buffer_free(packet);
}

static void *route_thread(void *arg)
{
    (void)arg;

    csp_route_work();

    return NULL;
}

static void *unbind_thread(void *arg)
{
    (void)arg;

    pthread_mutex_lock(&callback_mutex);

    unbind_started = true;
    pthread_cond_broadcast(&callback_cond);

    pthread_mutex_unlock(&callback_mutex);

    int result = csp_unbind_callback(TEST_CALLBACK_PORT);

    pthread_mutex_lock(&callback_mutex);

    unbind_result = result;
    unbind_completed = true;

    pthread_cond_broadcast(&callback_cond);

    pthread_mutex_unlock(&callback_mutex);

    return NULL;
}

START_TEST(test_unbind_waits_for_active_callback)
{
    pthread_t router;
    pthread_t unbinder;

    csp_init();

    pthread_mutex_lock(&callback_mutex);

    callback_entered = false;
    callback_release = false;
    unbind_started = false;
    unbind_completed = false;
    unbind_result = CSP_ERR_NONE;

    pthread_mutex_unlock(&callback_mutex);

    ck_assert_int_eq(csp_bind_callback(blocking_callback,
                                       TEST_CALLBACK_PORT),
                     CSP_ERR_NONE);

    send_to_port(TEST_CALLBACK_PORT);

    /* Start routing in a separate thread. */
    ck_assert_int_eq(pthread_create(&router, NULL, route_thread, NULL), 0);

    /* Wait until the callback is actually running. */
    pthread_mutex_lock(&callback_mutex);

    while (!callback_entered) {
        pthread_cond_wait(&callback_cond, &callback_mutex);
    }

    pthread_mutex_unlock(&callback_mutex);

    /* Start unbind while the callback is still blocked. */
    ck_assert_int_eq(pthread_create(&unbinder, NULL, unbind_thread, NULL), 0);

    pthread_mutex_lock(&callback_mutex);

    while (!unbind_started) {
        pthread_cond_wait(&callback_cond, &callback_mutex);
    }

    /*
     * Give the unbind thread time to execute.
     * It must not complete while the callback is active.
     */
    struct timespec deadline;

    clock_gettime(CLOCK_REALTIME, &deadline);

    deadline.tv_nsec += 100 * 1000 * 1000;

    if (deadline.tv_nsec >= 1000000000L) {
        deadline.tv_sec++;
        deadline.tv_nsec -= 1000000000L;
    }

    while (!unbind_completed) {
        int ret = pthread_cond_timedwait(&callback_cond,
                                         &callback_mutex,
                                         &deadline);

        if (ret == ETIMEDOUT) {
            break;
        }

        ck_assert_int_eq(ret, 0);
    }

    bool completed_while_callback_active = unbind_completed;

    /* Allow the callback to finish. */
    callback_release = true;
    pthread_cond_broadcast(&callback_cond);

    pthread_mutex_unlock(&callback_mutex);

    pthread_join(router, NULL);
    pthread_join(unbinder, NULL);

    ck_assert(!completed_while_callback_active);
    ck_assert(unbind_completed);
    ck_assert_int_eq(unbind_result, CSP_ERR_NONE);
}
END_TEST


Suite *route_suite(void)
{
    Suite *s = suite_create("route");

    TCase *tc = tcase_create("route");

    tcase_add_test(tc, test_bind_no_listen_conn_less_764);
    tcase_add_test(tc, test_bind_no_listen_conn_764);

    tcase_add_test(tc, test_bind_callback);
    tcase_add_test(tc, test_unbind_callback);
    tcase_add_test(tc, test_unbind_waits_for_active_callback);

    suite_add_tcase(s, tc);

    return s;
}