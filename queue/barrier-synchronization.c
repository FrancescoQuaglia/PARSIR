
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <queue.h>



long control_counter __attribute__((aligned(64))) = THREADS;
long era_counter __attribute__((aligned(64))) = THREADS;

int barrier(void){

	int ret;

	while(era_counter != THREADS && control_counter == THREADS);

	ret = __sync_bool_compare_and_swap(&control_counter,THREADS,0);
	if( ret ) era_counter = 0;

	__sync_fetch_and_add(&control_counter,1);
	while(control_counter != THREADS);
	__sync_fetch_and_add(&era_counter,1);

	return ret;
	
}

