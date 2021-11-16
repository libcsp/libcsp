#define CSP_USE_ASSERT 1  // always enable CSP assert

#include <csp/csp_debug.h>
#include <csp/arch/csp_sleep.h>
#include <csp/arch/csp_clock.h>
#include <csp/arch/csp_time.h>
#include <csp/arch/csp_queue.h>
#include <csp/arch/csp_semaphore.h>

#include <stdlib.h>

void csp_assert_fail_action(const char *assertion, const char *file, int line) {
    printf("assertion: [%s], file: %s:%d\r\n", assertion, file, line);
    exit(1);
}

int main(int argc, char * argv[]) {

    // debug/log - enable all levels
    for (int i = 0; i <= CSP_LOCK; ++i) {
        csp_debug_set_level(i, true);
    }
    csp_log_error("csp_log_error(...), level: %d", CSP_ERROR);
    csp_log_warn("csp_log_warn(...), level: %d", CSP_WARN);
    csp_log_info("csp_log_info((...), level: %d", CSP_INFO);
    csp_log_buffer("csp_log_buffer(...), level: %d", CSP_BUFFER);
    csp_log_packet("csp_log_packet(...), level: %d", CSP_PACKET);
    csp_log_protocol("csp_log_protocol(...), level: %d", CSP_PROTOCOL);
    csp_log_lock("csp_log_lock(...), level: %d", CSP_LOCK);

    // clock
    csp_timestamp_t csp_clock = {};
    csp_clock_get_time(&csp_clock);
    csp_assert(csp_clock.tv_sec != 0);
    csp_log_info("csp_clock_get_time(..) -> sec:nsec = %"PRIu32":%"PRIu32, csp_clock.tv_sec, csp_clock.tv_nsec);

    // relative time
    const uint32_t msec1 = csp_get_ms();
    const uint32_t msec2 = csp_get_ms_isr();
    const uint32_t sec1 = csp_get_s();
    const uint32_t sec2 = csp_get_s_isr();
    csp_sleep_ms(2000);
    csp_assert(csp_get_ms() >= (msec1 + 500));
    csp_assert(csp_get_ms_isr() >= (msec2 + 500));
    csp_assert(csp_get_s() >= (sec1 + 1));
    csp_assert(csp_get_s_isr() >= (sec2 + 1));

    // queue handling
    uint32_t value;
    csp_queue_handle_t q = csp_queue_create(3, sizeof(value));
    csp_assert(csp_queue_size(q) == 0);
    csp_assert(csp_queue_size_isr(q) == 0);
    csp_assert(csp_queue_dequeue(q, &value, 0) == CSP_QUEUE_ERROR);
    csp_assert(csp_queue_dequeue(q, &value, 200) == CSP_QUEUE_ERROR);
    csp_assert(csp_queue_dequeue_isr(q, &value, NULL) == CSP_QUEUE_ERROR);
    value = 1;
    csp_assert(csp_queue_enqueue(q, &value, 0) == CSP_QUEUE_OK);
    value = 2;
    csp_assert(csp_queue_enqueue(q, &value, 200) == CSP_QUEUE_OK);
    value = 3;
    csp_assert(csp_queue_enqueue_isr(q, &value, NULL) == CSP_QUEUE_OK);
    csp_assert(csp_queue_size(q) == 3);
    csp_assert(csp_queue_size_isr(q) == 3);
    value = 10;
    csp_assert(csp_queue_enqueue(q, &value, 0) == CSP_QUEUE_ERROR);
    value = 20;
    csp_assert(csp_queue_enqueue(q, &value, 200) == CSP_QUEUE_ERROR);
    value = 30;
    csp_assert(csp_queue_enqueue_isr(q, &value, NULL) == CSP_QUEUE_ERROR);
    value = 100;
    csp_assert(csp_queue_dequeue(q, &value, 0) == CSP_QUEUE_OK);
    csp_assert(value == 1);
    csp_assert(csp_queue_dequeue(q, &value, 200) == CSP_QUEUE_OK);
    csp_assert(value == 2);
    csp_assert(csp_queue_dequeue_isr(q, &value, NULL) == CSP_QUEUE_OK);
    csp_assert(value == 3);
    csp_queue_remove(q);


    // semaphore
    csp_bin_sem_handle_t s;
    csp_assert(csp_bin_sem_create(&s) == CSP_SEMAPHORE_OK);
    csp_assert(csp_bin_sem_wait(&s, 0) == CSP_SEMAPHORE_OK);
    csp_assert(csp_bin_sem_post(&s) == CSP_SEMAPHORE_OK);
#if (CSP_POSIX) // implementations differ in return value if already posted/signaled
    csp_assert(csp_bin_sem_post_isr(&s, NULL) == CSP_SEMAPHORE_OK);
#else
    csp_assert(csp_bin_sem_post_isr(&s, NULL) == CSP_SEMAPHORE_ERROR);
#endif
    csp_assert(csp_bin_sem_wait(&s, 200) == CSP_SEMAPHORE_OK);
    csp_assert(csp_bin_sem_wait(&s, 200) == CSP_SEMAPHORE_ERROR);
    csp_assert(csp_bin_sem_remove(&s) == CSP_SEMAPHORE_OK);

    return 0;
}
