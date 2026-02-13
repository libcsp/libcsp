#include <unistd.h>
#include <csp/csp_debug.h>
#include <csp/csp_types.h>
#include <csp/csp_hooks.h>
#include <csp/arch/csp_time.h>
#include <csp/arch/csp_queue.h>
#include <assert.h>
#include <stdlib.h>

#ifdef NDEBUG
#define ASSERT(expr) ((void)(expr))
#else
#define ASSERT(expr) assert(expr)
#endif

void csp_panic(const char * msg) {
    csp_print("csp_panic: %s\n", msg);
    exit(1);
}

int main(void) {

    // clock
    csp_timestamp_t csp_clock = {0};
    csp_clock_get_time(&csp_clock);
    ASSERT(csp_clock.tv_sec != 0);
    csp_print("csp_clock_get_time(..) -> sec:nsec = %"PRIu32":%"PRIu32"\n", csp_clock.tv_sec, csp_clock.tv_nsec);

    // relative time
    const uint32_t msec1 = csp_get_ms();
    const uint32_t msec2 = csp_get_ms_isr();
    const uint32_t sec1 = csp_get_s();
    const uint32_t sec2 = csp_get_s_isr();
    sleep(2);
    ASSERT(csp_get_ms() >= (msec1 + 500));
    ASSERT(csp_get_ms_isr() >= (msec2 + 500));
    ASSERT(csp_get_s() >= (sec1 + 1));
    ASSERT(csp_get_s_isr() >= (sec2 + 1));

    // queue handling
    uint32_t value;
    csp_static_queue_t sq;
    csp_queue_handle_t q;
    char buf[3 * sizeof(value)];
    q = csp_queue_create_static(3, sizeof(value), buf, &sq);
    ASSERT(csp_queue_size(q) == 0);
    ASSERT(csp_queue_size_isr(q) == 0);
    ASSERT(csp_queue_dequeue(q, &value, 0) == CSP_QUEUE_ERROR);
    ASSERT(csp_queue_dequeue(q, &value, 200) == CSP_QUEUE_ERROR);
    ASSERT(csp_queue_dequeue_isr(q, &value, NULL) == CSP_QUEUE_ERROR);
    value = 1;
    ASSERT(csp_queue_enqueue(q, &value, 0) == CSP_QUEUE_OK);
    value = 2;
    ASSERT(csp_queue_enqueue(q, &value, 200) == CSP_QUEUE_OK);
    value = 3;
    ASSERT(csp_queue_enqueue_isr(q, &value, NULL) == CSP_QUEUE_OK);
    ASSERT(csp_queue_size(q) == 3);
    ASSERT(csp_queue_size_isr(q) == 3);
    value = 10;
    ASSERT(csp_queue_enqueue(q, &value, 0) == CSP_QUEUE_ERROR);
    value = 20;
    ASSERT(csp_queue_enqueue(q, &value, 200) == CSP_QUEUE_ERROR);
    value = 30;
    ASSERT(csp_queue_enqueue_isr(q, &value, NULL) == CSP_QUEUE_ERROR);
    value = 100;
    ASSERT(csp_queue_dequeue(q, &value, 0) == CSP_QUEUE_OK);
    ASSERT(value == 1);
    ASSERT(csp_queue_dequeue(q, &value, 200) == CSP_QUEUE_OK);
    ASSERT(value == 2);
    ASSERT(csp_queue_dequeue_isr(q, &value, NULL) == CSP_QUEUE_OK);
    ASSERT(value == 3);

    return 0;
}
