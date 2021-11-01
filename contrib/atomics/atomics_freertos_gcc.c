/**
 * CSP Uses GCC C11 atomics to perform atomic compare and exchange, on some platforms this
 * is not supported natively by the compiler. In those cases the compiler issues a function
 * call, which this file provide examples of.
 * 
 * @brief Implementation of C11 atomic operations if GCC does not provide an
 * implementation.
 *
 * GCC with -mcpu=cortex-m3 and cortex-m4 generate LDREX/STREX instructions
 * instead of library calls. There is however currently (2015-05-29) no
 * implementation for Cortex-M0, since it lacks the lock-free atomic operations
 * found in the M3 and M4 cores.
 *
 * @note Other CPUs (e.g. msp430, avr) might need this too, but current MSP430
 * GCC in Ubuntu/Debian is stuck at version 4.6 which does not provide C11
 * language support which makes it difficult to actually make use of this on
 * that platform.
 *
 * @note This implementation completely ignores the memory model parameter
 *
 * @see https://gcc.gnu.org/wiki/Atomic/GCCMM/LIbrary
 * @see https://gcc.gnu.org/onlinedocs/gcc/_005f_005fatomic-Builtins.html
 * 
 */

#include <FreeRTOS.h>
#include <stdbool.h>

bool __atomic_compare_exchange_4(volatile void *ptr, void *expected, unsigned int desired, bool weak, int success_memorder, int failure_memorder) {
    bool ret = false;

    portENTER_CRITICAL();
    
    if (*(unsigned int *)ptr == *(unsigned int *)expected) {
        *(unsigned int *)ptr = desired;
        ret = true;
    } else {
        *(unsigned int *)expected = *(unsigned int *)ptr;
        ret = false;
    }

    portEXIT_CRITICAL();

    return ret;
}
