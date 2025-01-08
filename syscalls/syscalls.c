#include <stdio.h>

#ifndef SYSCALL_NUMBER
#define SYSCALL_NUMBER 134 //this is the first syscall table entry available on Linux/x86-64 at date January 2025 according to the Posix standard
#pragma message("System call number set to the default value of 134")
#endif


#define _GNU_SOURCE         /* See feature_test_macros(7) */
       #include <unistd.h>
       #include <sys/syscall.h>

int switch_kernel_array_entries(unsigned long address, int indexA, int indexB){
//	printf("syscall - asked to switch at address %p - entry %d with entry %d\n",(void*)address,indexA,indexB);
//	fflush(stdout);
	syscall(SYSCALL_NUMBER,address,indexA,indexB);
	return -1;//the return value is currently a do not care
}
