
#include <zephyr.h>
#include <csp/csp_debug.h>
#include <csp/arch/csp_clock.h>
#include <csp/arch/csp_time.h>
#include <csp/arch/csp_queue.h>
#include <csp/arch/csp_semaphore.h>
#include <unistd.h>

#include <stdlib.h>

static bool thread_executed = false;

void thread_func(void * p1, void * p2, void * p3) {
    thread_executed = true;
    sleep(10); // safty - ensure process terminates
    exit(1);
    return;
}

#define STACKSIZE 1024
K_THREAD_STACK_DEFINE(stack, STACKSIZE);

int main(int argc, char * argv[]) {

    // debug/log - enable all levels
    for (int i = 0; i <= CSP_LOCK; ++i) {
        csp_debug_set_level(i, true);
    }

    // create a thread
    k_tid_t tid;
    struct k_thread new_thread;

    tid = k_thread_create(&new_thread,
						  stack, K_THREAD_STACK_SIZEOF(stack),
						  thread_func, NULL, NULL, NULL,
						  0, 0, K_NO_WAIT);
    assert(tid != NULL);

    // clock
    csp_timestamp_t csp_clock = {};
    csp_clock_get_time(&csp_clock);
    assert(csp_clock.tv_sec != 0);
    printf("csp_clock_get_time(..) -> sec:nsec = %"PRIu32":%"PRIu32"\n", csp_clock.tv_sec, csp_clock.tv_nsec);

    // relative time
    const uint32_t msec1 = csp_get_ms();
    const uint32_t msec2 = csp_get_ms_isr();
    const uint32_t sec1 = csp_get_s();
    const uint32_t sec2 = csp_get_s_isr();
    sleep(2);

    assert(csp_get_ms() >= (msec1 + 500));
    assert(csp_get_ms_isr() >= (msec2 + 500));
    assert(csp_get_s() >= (sec1 + 1));
    assert(csp_get_s_isr() >= (sec2 + 1));

    // check thread actually executed
    assert(thread_executed != false);

    // queue handling
    uint32_t value;
    csp_static_queue_t sq;
    csp_queue_handle_t q;
    char buf[3 * sizeof(value)];
    q = csp_queue_create_static(3, sizeof(value), buf, &sq);
    assert(csp_queue_size(q) == 0);
    assert(csp_queue_size_isr(q) == 0);
    assert(csp_queue_dequeue(q, &value, 0) == CSP_QUEUE_ERROR);
    assert(csp_queue_dequeue(q, &value, 200) == CSP_QUEUE_ERROR);
    assert(csp_queue_dequeue_isr(q, &value, NULL) == CSP_QUEUE_ERROR);
    value = 1;
    assert(csp_queue_enqueue(q, &value, 0) == CSP_QUEUE_OK);
    value = 2;
    assert(csp_queue_enqueue(q, &value, 200) == CSP_QUEUE_OK);
    value = 3;
    assert(csp_queue_enqueue_isr(q, &value, NULL) == CSP_QUEUE_OK);
    assert(csp_queue_size(q) == 3);
    assert(csp_queue_size_isr(q) == 3);
    value = 10;
    assert(csp_queue_enqueue(q, &value, 0) == CSP_QUEUE_ERROR);
    value = 20;
    assert(csp_queue_enqueue(q, &value, 200) == CSP_QUEUE_ERROR);
    value = 30;
    assert(csp_queue_enqueue_isr(q, &value, NULL) == CSP_QUEUE_ERROR);
    value = 100;
    assert(csp_queue_dequeue(q, &value, 0) == CSP_QUEUE_OK);
    assert(value == 1);
    assert(csp_queue_dequeue(q, &value, 200) == CSP_QUEUE_OK);
    assert(value == 2);
    assert(csp_queue_dequeue_isr(q, &value, NULL) == CSP_QUEUE_OK);
    assert(value == 3);


    // semaphore
    csp_bin_sem_handle_t s;
    csp_bin_sem_create_static(&s, NULL);
    assert(csp_bin_sem_wait(&s, 0) == CSP_SEMAPHORE_OK);
    assert(csp_bin_sem_post(&s) == CSP_SEMAPHORE_OK);
#if (CSP_POSIX || CSP_ZEPHYR) // implementations differ in return value if already posted/signaled
    assert(csp_bin_sem_post_isr(&s, NULL) == CSP_SEMAPHORE_OK);
#else
    assert(csp_bin_sem_post_isr(&s, NULL) == CSP_SEMAPHORE_ERROR);
#endif
    assert(csp_bin_sem_wait(&s, 200) == CSP_SEMAPHORE_OK);
    assert(csp_bin_sem_wait(&s, 200) == CSP_SEMAPHORE_ERROR);

    return 0;
}
