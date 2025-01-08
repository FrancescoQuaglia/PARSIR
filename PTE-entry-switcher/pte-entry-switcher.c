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
* @file pte-entry-switcher.c 
* @brief This is the main source for the Linux Kernel Module which implements
*       a system call that can be used to switch the logical to physical mapping 
* 	of two contiguous mmapped zones, each of 512 pages	
*	This module relies on the https://github.com/FrancescoQuaglia/Linux-sys_call_table-discoverer.git
	Linux kernel module for the identification of the positioning of the system call table
*
* @author Francesco Quaglia
*
* @date March 25, 2024
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
#include <asm/io.h>
#include <linux/syscalls.h>
#include <asm/tlbflush.h>
#include "lib/include/scth.h"




MODULE_LICENSE("GPL");
MODULE_AUTHOR("Francesco Quaglia <francesco.quaglia@uniroma2.it>");
MODULE_DESCRIPTION("PTE entry switcher - this version does not switch bitmasks for mempolicies, it only switches PDE entries in the address space");

#define MODNAME "PES"


unsigned long the_syscall_table = 0x0;
module_param(the_syscall_table, ulong, 0660);

unsigned long the_syscall_number = -1;
module_param(the_syscall_number, long, 0660);


unsigned long the_ni_syscall;

unsigned long new_sys_call_array[] = {0x0};//please set to sys_pte_entry_switch at startup
#define HACKED_ENTRIES (int)(sizeof(new_sys_call_array)/sizeof(unsigned long))
int restore[HACKED_ENTRIES] = {[0 ... (HACKED_ENTRIES-1)] -1};


//stuff for actual service operations
#define ADDRESS_MASK 0xfffffffffffff000
#define PAGE_TABLE_ADDRESS phys_to_virt(_read_cr3() & ADDRESS_MASK)
#define PT_ADDRESS_MASK 0x7ffffffffffff000
#define VALID 0x1
#define MODE 0x100
#define LH_MAPPING 0x80
#define IS_USER 0x4

#define PML4(addr) (((long long)(addr) >> 39) & 0x1ff)
#define PDP(addr)  (((long long)(addr) >> 30) & 0x1ff)
#define PDE(addr)  (((long long)(addr) >> 21) & 0x1ff)
#define PTE(addr)  (((long long)(addr) >> 12) & 0x1ff)

#define NO_MAP (-EFAULT)

#define AUDIT if(0)

//auxiliary stuff
static inline unsigned long _read_cr3(void) {

          unsigned long val;
          asm volatile("mov %%cr3,%0":  "=r" (val) : );
          return val;

}

//#define TLB_TEST

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
__SYSCALL_DEFINEx(1, _pte_entry_switch, unsigned long, vaddr){
#else
asmlinkage long sys_pte_entry_switch(unsigned long vaddr, int PTE_index_A, int PTE_index_B){
#endif
        void* target_address;
        
	pud_t *pdp;
	pmd_t *pde;
	pmd_t pte_slot;
	pgd_t *pml4;
	

	target_address = (void*)vaddr; 
        /* fixing the address to use for page table access */       

	AUDIT{
	printk("%s: ------------------------------\n",MODNAME);
	printk("%s: asked to swtich PTE entries with base address %px - indexes %d - %d\n",MODNAME,(target_address), PTE_index_A, PTE_index_B);
	}

	if ((PTE_index_A < 0) || (PTE_index_B < 0)) {
		printk("%s: invalid indexes\n",MODNAME);
		return -EINVAL;	
	} 


	//finalize indexing starting from the PDE entry associated with the target address
	PTE_index_A += PDE(target_address);	
	PTE_index_B += PDE(target_address);	

	if ((PTE_index_A > 511) || PTE_index_B > 511){
		printk("%s: invalid indexes\n",MODNAME);
		return -EINVAL;	
	}	

	//start page table traversing
	pml4  = PAGE_TABLE_ADDRESS;

	down_read(&(current->mm->mmap_sem));

	AUDIT printk("%s: PML4 traversing on entry %lld\n",MODNAME,PML4(target_address));

	if(((ulong)(pml4[PML4(target_address)].pgd)) & VALID){
	}
	else{
		AUDIT printk("%s: PML4 region not mapped to physical memory\n",MODNAME);
		up_read(&(current->mm->mmap_sem));
		return NO_MAP;
	} 

	pdp = __va((ulong)(pml4[PML4(target_address)].pgd) & PT_ADDRESS_MASK);           

	AUDIT
	printk("%s: PDP traversing on entry %lld\n",MODNAME,PDP(target_address));

	if((ulong)(pdp[PDP(target_address)].pud) & VALID){
	}
	else{ 
		AUDIT printk("%s: PDP region not mapped to physical memory\n",MODNAME);
		up_read(&(current->mm->mmap_sem));
		return NO_MAP;
	}

	pde = __va((ulong)(pdp[PDP(target_address)].pud) & PT_ADDRESS_MASK);
	AUDIT printk("%s: PDE traversing on entry %lld\n",MODNAME,PDE(target_address));
	
#ifdef TLB_TEST
	__native_flush_tlb();
#else
	if( ((ulong)(pde[PTE_index_B].pmd) & VALID) && 
	    ((ulong)(pde[PTE_index_B].pmd) & IS_USER) &&
	    ((ulong)(pde[PTE_index_A].pmd) & VALID) &&
	    ((ulong)(pde[PTE_index_A].pmd) & IS_USER) ){
			pte_slot = pde[PTE_index_B];
			pde[PTE_index_B] = pde[PTE_index_A];
			pde[PTE_index_A] = pte_slot;
			__native_flush_tlb();
			AUDIT printk("%s: PDE entries %d - %d have been switched\n",MODNAME,PTE_index_A, PTE_index_B);

	}
#endif

	up_read(&(current->mm->mmap_sem));

	return 0;

}


#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
long sys_pte_entry_switch = (unsigned long) __x64_sys_pte_entry_switch;       
#else
#endif


int init_module(void) {

        int i;
        int ret;

	if (the_syscall_table == 0x0) {
	   printk("%s: PTE entry switcher started up with invalid sys_call_table address set to %px\n",MODNAME,(void*)the_syscall_table);
		return -1;
	}

	AUDIT{
	   printk("%s: PTE entry switcher received sys_call_table address %px\n",MODNAME,(void*)the_syscall_table);
     	   printk("%s: initializing - hacked entries %d\n",MODNAME,HACKED_ENTRIES);
	}

	new_sys_call_array[0] = (unsigned long)sys_pte_entry_switch;

        ret = get_entries(restore,HACKED_ENTRIES,(unsigned long*)the_syscall_table,&the_ni_syscall);

        if (ret != HACKED_ENTRIES){
                printk("%s: could not hack %d entries (just %d)\n",MODNAME,HACKED_ENTRIES,ret); 
                return -1;      
        }

	unprotect_memory();

        for(i=0;i<HACKED_ENTRIES;i++){
                ((unsigned long *)the_syscall_table)[restore[i]] = (unsigned long)new_sys_call_array[i];
		the_syscall_number = restore[i];
        }

	protect_memory();

        printk("%s: all new system-calls correctly installed on sys-call table\n",MODNAME);

        return 0;

}

void cleanup_module(void) {

        int i;
                
        printk("%s: shutting down\n",MODNAME);

	unprotect_memory();
        for(i=0;i<HACKED_ENTRIES;i++){
                ((unsigned long *)the_syscall_table)[restore[i]] = the_ni_syscall;
        }
	protect_memory();
        printk("%s: sys-call table restored to its original content\n",MODNAME);
        
}
