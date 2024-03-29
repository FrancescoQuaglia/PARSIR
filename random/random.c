#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

/**
* This function returns a number in between (0,1), according to a Uniform Distribution.
* It is based on Multiply with Carry by George Marsaglia
*/

double Random(uint32_t *seed1, uint32_t *seed2) {

	*seed1 = 36969u * (*seed1 & 0xFFFFu) + (*seed1 >> 16u);
	*seed2 = 18000u * (*seed2 & 0xFFFFu) + (*seed2 >> 16u);

	// The magic number below is 1/(2^32 + 2).
    	// The result is strictly between 0 and 1.
	return (((*seed1 << 16u) + (*seed1 >> 16u) + *seed2) + 1.0) * 2.328306435454494e-10;
}

double Expent(double mean, uint32_t *seed1, uint32_t *seed2) {

	if(mean < 0) {
		printf("error - call to Expent() has a negative mean value\n");
		exit(EXIT_FAILURE);
	}

	return mean * ( 1.0 / ( 3 *(1.01 - Random(seed1,seed2)) ) );
}
