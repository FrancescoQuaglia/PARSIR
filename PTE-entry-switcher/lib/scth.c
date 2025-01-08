/*
* 
* This is free software; you can redistribute it and/or modify it under the
* terms of the GNU General Public License as published by the Free Software
* Foundation; either version 3 of the License, or (at your option) any later
* version.
* 
* This module is distributed in the hope that it will be useful, but WITHOUT ANY
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
* A PARTICULAR PURPOSE. See the GNU General Public License for more details.
* 
* @file scth.c 
* @brief This is the main source for the Linux Kernel Lib  which implements
* 	 the dynamis discovery of the position of the sys_call_table and the identifiction of
* 	 entries pointing to sys_ni_syscall - these entries are returned together with the address
* 	 of the sys_call_table and the address of sys_ni_syscall so that they can be exploited for
* 	 rewriting sys_call_table and adding new system calls to Linux
*
* @author Francesco Quaglia
*
* @date October 20, 2019
*/

#define EXPORT_SYMTAB
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/kprobes.h>
#include <linux/mutex.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <asm/page.h>
#include <asm/cacheflush.h>
#include <asm/apic.h>
#include <linux/syscalls.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Francesco Quaglia <quaglia@dis.uniroma1.it>");
MODULE_DESCRIPTION("system call table hacker");

#define LIBNAME "SCTH"


#define AUDIT if(1)
#define LEVEL3_AUDIT if(0)

#define MAX_ACQUIRES 4


//stuff for sys cal table hacking
#if LINUX_VERSION_CODE > KERNEL_VERSION(3,3,0)
    #include <asm/switch_to.h>
#else
    #include <asm/system.h>
#endif
#ifndef X86_CR0_WP
#define X86_CR0_WP 0x00010000
#endif

unsigned long cr0;
static inline void write_cr0_forced(unsigned long val) {
    unsigned long __force_order;

    /* __asm__ __volatile__( */
    asm volatile(
        "mov %0, %%cr0"
        : "+r"(val), "+m"(__force_order));
}

void protect_memory(void){
    write_cr0_forced(cr0);
}

void unprotect_memory(void){
    cr0 = read_cr0();
    write_cr0_forced(cr0 & ~X86_CR0_WP);
}



int get_entries(int * entry_ids, int num_acquires, unsigned long sys_call_table, unsigned long *sys_ni_sys_call) {

        unsigned long * p;
        unsigned long addr;
        int i,j,z,k; //stuff to discover memory contents
        int ret = 0;
	int restore[MAX_ACQUIRES] = {[0 ... (MAX_ACQUIRES-1)] -1};


        printk("%s: trying to get %d entries from the sys-call table at address %px\n",LIBNAME,num_acquires, (void*)sys_call_table);
	if(num_acquires < 1){
       		 printk("%s: less than 1 sys-call table entry requested\n",LIBNAME);
		 return -1;
	}
	if(num_acquires > MAX_ACQUIRES){
       		 printk("%s: more than %d sys-call table entries requested\n",LIBNAME, MAX_ACQUIRES);
		 return -1;
	}

	p = (unsigned long*)sys_call_table;

        j = -1;
        for (i=0; i<256; i++){
		for(z=i+1; z<256; z++){
			if(p[i] == p[z]){
				AUDIT{
                        		printk("%s: table entries %d and %d keep the same address\n",LIBNAME,i,z);
                        		printk("%s: sys_ni_syscall correctly located at %px\n",LIBNAME,(void*)p[i]);
				}
				addr = p[i];
                        	if(j < (num_acquires-1)){
				       	restore[++j] = i;
					ret++;
                        		printk("%s: acquiring table entry %d\n",LIBNAME,i);
				}
                        	if(j < (num_acquires-1)){
                        		restore[++j] = z;
					ret++;
                        		printk("%s: acquiring table entry %d\n",LIBNAME,z);
				}
				for(k=z+1;k<256 && j < (num_acquires-1); k++){
					if(p[i] == p[k]){
                        			printk("%s: acquiring table entry %d\n",LIBNAME,k);
                        			restore[++j] = k;
						ret++;
					}
				}
				if(ret == num_acquires){
					goto found_available_entries;
				}
				return -1;	
			}
                }
        }

        printk("%s: could not locate %d available entries in the sys-call table\n",LIBNAME,num_acquires);

	return -1;

found_available_entries:
        printk("%s: ret is %d\n",LIBNAME,ret);
	memcpy((char*)entry_ids,(char*)restore,ret*sizeof(int));
	*sys_ni_sys_call = addr;

	return ret;

}


