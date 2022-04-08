#include <stdlib.h>
#include <csp/arch/csp_rand.h>
#include <random/rand32.h>

uint8_t csp_rand8(void)
{
    return sys_rand32_get();
}

uint16_t csp_rand16(void)
{
    return sys_rand32_get();
}
