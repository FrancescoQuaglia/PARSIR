
#include "setup.h"

//#ifdef NUMA_BALANCING
//#define NUMA_UBIQUITOUS //for now we are still subject to this define
//#endif


#ifdef NUMA_UBIQUITOUS 
  #ifndef NUMA_BALANCING 
    #error "NUMA_UBIQUITOUS defined but NUMA_BALANCING not defined"
  #endif
#endif

#ifdef TLB_TEST 
  #ifndef NUMA_UBIQUITOUS 
    #error "TLB_TEST defined but NUMA_UBIQUITOUS not defined"
  #endif
#endif



typedef struct _numa_map {
        int primary_node;//this is the NUMA node ID where the object is initially placed
        unsigned long base_address;//this is the logical address of the
                                   //memory segment for the object allocator
        int node_indexing[MEM_NODES];//this array tells the index of the PTE array
                                     //entry for a memory region related to a NUMA node
} numa_map;


void allocators_base_init(void);

void object_allocator_setup(void);

unsigned long get_kernel_array_address(unsigned long);

int switch_kernel_array_entries(unsigned long address, int indexA, int indexB);

void sanitize(int);

