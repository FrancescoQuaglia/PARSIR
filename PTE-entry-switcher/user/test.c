#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/mman.h>
#include <pthread.h>


#define PES 134 //this depends on what the kernel tells you when mounting the vtpmo module

int pes(unsigned long x, int a, int b){
	return syscall(PES,x,a,b);
}

#define NUM_TARGET_PAGES 512
#define ZONE_SIZE (4096 * NUM_TARGET_PAGES)

//unsigned long base = (4 *(1<<20));
#define BASE (2048 * 4 * 1024)
unsigned long base = BASE;
unsigned long base1 = BASE + 512 * 4 * 1024;;


int main(int argc, char** argv){
	char * zone_A = NULL;
	char * zone_B = NULL;

	printf("base is %p - base1 is %p - zone side is %p\n",base,base1, (void*)ZONE_SIZE);
	fflush(stdout);


	zone_A = (char*)mmap((void*)base, ZONE_SIZE, PROT_READ|PROT_WRITE, MAP_FIXED|MAP_ANONYMOUS|MAP_PRIVATE,0,0);
	zone_B = (char*)mmap((char*)base1, ZONE_SIZE, PROT_READ|PROT_WRITE, MAP_FIXED|MAP_ANONYMOUS|MAP_PRIVATE,0,0);
        if (zone_A == NULL || zone_B == NULL){
		printf("memory allocation error\n");
		fflush(stdout);
		exit(EXIT_FAILURE);
        } 

	sprintf(zone_A,"%s","Francesco");
	sprintf(zone_B,"%s","Quaglia");

redo:
	printf("zone A has content: %s\n",zone_A);
	printf("zone B has content: %s\n",zone_B);
	fflush(stdout);

	pes(base,0,1);

	//sleep(1);

	printf("zone A has content: %s\n",zone_A);
	printf("zone B has content: %s\n",zone_B);
	fflush(stdout);

	pes(base,1,0);

	//sleep(1);

	printf("zone A has content: %s\n",zone_A);
	printf("zone B has content: %s\n",zone_B);
	fflush(stdout);

goto redo;

	return 0;

}

