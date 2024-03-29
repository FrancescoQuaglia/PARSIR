
#include <stdint.h>
#include <stdlib.h>
#include <params.h>

#define FLT_EPSILON 1.19209290E-07F // decimal constant

typedef double simtime_t;
typedef int bool;

#define false 0
#define true 1


#define SIZE1 32
typedef union _datatye1{
        void * p;
        char buff[SIZE1];
} datatype1;

#define SIZE2 64
typedef union _datatype2{
        void * p;
        char buff[SIZE1];
} datatype2;



/* DISTRIBUZIONI TIMESTAMP */
#define UNIFORM		0
#define EXPONENTIAL	1

//event types
#define NORMAL 1

typedef struct _lp_state_type{
	int event_count;
	uint32_t seed1;//seed passed in input to randomization functions
       	uint32_t seed2;//seed passed in input to randomization functions
        datatype1 * p1;
        datatype2 * p2;
} lp_state_type;



