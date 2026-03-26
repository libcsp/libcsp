#include <stdint.h>

#define RAND_R_MULT 16807
#define RAND_R_MOD  2147483647

int rand_r(unsigned int * seedp) {
	const uint64_t result = ((uint64_t)*seedp * RAND_R_MULT) % RAND_R_MOD;

	return (unsigned int)result;
}
