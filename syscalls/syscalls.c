#include <stdio.h>

int switch_kernel_array_entries(unsigned long address, int indexA, int indexB){
	printf("syscall - asked to switch at address %p - entry %d with entry %d\n",(void*)address,indexA,indexB);
	return -1;
}
