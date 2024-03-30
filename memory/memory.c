#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <numaif.h>
#include "setup.h"
#include "queue.h"
#include "memory.h"

#define MIN_CHUNK_SIZE (32)
#define MAX_CHUNK_SIZE (MIN_CHUNK_SIZE << 7)//4KB is the currently set max chunk size
#define EPSILON 0.000001

unsigned long MAX_MEMORY = (1<<21);//maximum amount of memory manageable per object

unsigned long base_address;

unsigned long SEGMENTS;
unsigned long SEGMENT_PAGES;

typedef struct _area{
	void ** addresses;
	int top_elem;
	int size;
} area;

area * allocators[OBJECTS];
void * base[OBJECTS];

#define NUMA_UBIQUITOUS //for now we are still subject to this define

#ifdef NUMA_UBIQUITOUS
typedef struct _numa_map {
	int primary_node;//this is the NUMA node ID where the object is initially placed
	unsigned long base_address;//this is the logical address of the
				   //memory segment for the object allocator
	int node_indexing[MEM_NODES];//this array tells the index of the PTE array
				     //entry for a memory region related to a NUMA node
} numa_map;

numa_map maps[OBJECTS];
#endif

void allocators_base_init(void){
	int i;

	SEGMENTS = MAX_MEMORY >> 3;//each segment is configured to be a set of 64 pages
	SEGMENT_PAGES = SEGMENTS >> 12;
	AUDIT printf("base allocators init - max memory is %ld - segment size is %ld - segment pages are %ld - memory areas are %d\n",MAX_MEMORY,SEGMENTS,SEGMENT_PAGES,(int)((double)(MAX_MEMORY>>12)/(double)(SEGMENT_PAGES)));
	for (i = 0; i < OBJECTS; i++){
		allocators[i] = malloc((int)((double)(MAX_MEMORY>>12)/(double)(SEGMENT_PAGES)) * sizeof(area));
		if (!allocators[i]){
			printf("allocators base init error\n");
			exit(EXIT_FAILURE);
		}
		base[i] = NULL;
	}

}

void object_allocator_setup(void){//size_t size){

	int i;
	int j;
	int current;
	int NUMAnode;
	unsigned long target_address;
	void* mapping;
	void* addr;
	int ret;
	unsigned long mask;
	int chunk_size;
	void* limit;
	void* aux;
	unsigned long kernel_array_address;

	base_address = 8 * (1024 * MAX_MEMORY);//this can be setup in a different manner if needed
	current = get_current();
	NUMAnode = get_NUMAnode();
	target_address = base_address + current * (MAX_MEMORY * MEM_NODES);

	AUDIT printf("allocator setup for object %d\n",current);

	mask = 0x1 << NUMAnode;//this is for NUMA binding of memory zones
	AUDIT printf("thread running on NUMA node %d - mask for setting up object %d is %lu\n",NUMAnode,current,mask);

#ifdef NUMA_UBIQUITOUS
	maps[current].primary_node = NUMAnode; 
	maps[current].base_address = target_address; 
	for (i = 0; i < MEM_NODES; i++){
#else
	for (i = 0; i < 1; i++){//the above line is left just to let the developer restart from here for NUMA ubiquitousness
#endif
		addr = mmap((void*)target_address, MAX_MEMORY, PROT_READ|PROT_WRITE, MAP_ANONYMOUS| MAP_PRIVATE | MAP_FIXED,0,0);
		if(addr != (void*)target_address){
			printf("mmap failure for object %d\n",current);
			exit(EXIT_FAILURE);
		}; 
		ret = mbind(addr,MAX_MEMORY,MPOL_BIND,&mask,sizeof(unsigned long),0);
		if(ret == -1 ){
			printf("mbibd failure for object %d\n",current);
			exit(EXIT_FAILURE);
		}; 
		*(char*)addr = 'f';//materialize the 3-rd level page table entry
		AUDIT printf("mapped zone at address %p for object %d\n",(void*)target_address,current);
#ifdef NUMA_UBIQUITOUS
		maps[current].node_indexing[NUMAnode] = i; 
		target_address += MAX_MEMORY;
		NUMAnode = NUMAnode + 1;
		NUMAnode = NUMAnode % MEM_NODES;
		mask = 0x1 << NUMAnode;
		AUDIT printf("new NUMA node for object %d is %p\n",current,(void*)mask);
#endif
	}

#ifdef NUMA_UBIQUITOUS
	AUDIT for (i = 0 ; i < MEM_NODES; i++){
		printf("indexing for object %d - index value %d - NUMA node is %d\n",current,i,maps[current].node_indexing[i]);
	}
#endif

        target_address = base_address + current * (MAX_MEMORY * MEM_NODES);
        base[current] = (void*)target_address;

	//the allocator is based on a stack of free-chunk addresses 
	//now we initialize the free stacks of the per-object allocator
	//the minimal chunk size managed is 32 bytes
 		
	AUDIT printf("running the allocator setup before iteration for object %d - cycles are %d\n",current,(int)((double)(MAX_MEMORY>>12)/(double)(SEGMENT_PAGES)));
	limit = base[current]; //(void*)target_address;
	for(i = 0, chunk_size = MIN_CHUNK_SIZE; i < (int)((double)(MAX_MEMORY>>12)/(double)(SEGMENT_PAGES)) ; chunk_size *= 2 , i++){

		AUDIT printf("running the allocator setup for object %d\n",current);

		(allocators[current])[i].addresses = malloc(sizeof(void*)*((SEGMENT_PAGES << 12) / chunk_size));
		if ( ! (allocators[current])[i].addresses ){
			printf("area allocation error\n");
			exit(EXIT_FAILURE);
		}
		AUDIT printf("current is %d - allocated area with %ld elements (chunk size is %d)\n", current, (SEGMENT_PAGES << 12) / chunk_size, chunk_size);
		for(j = 0; j < ((SEGMENT_PAGES << 12) / chunk_size); j++){
			(allocators[current])[i].addresses[j] = limit;
			limit += chunk_size;

		}
		(allocators[current])[i].top_elem = 0;
		(allocators[current])[i].size = ((SEGMENT_PAGES << 12) / chunk_size);
	}

}

void* __wrap_malloc(size_t size){
	int current;
	int index;
	void * chunk_address;

	current = get_current();
	if (size == 0) return NULL;
	index = (int)( ((double)size / (double)MIN_CHUNK_SIZE) - EPSILON);
	AUDIT printf("wrapping malloc for object %d - size is %ld - index is %d\n",current,size,index);
redo:
	if(index >= (int)((double)(MAX_MEMORY>>12)/(double)(SEGMENT_PAGES))){
		printf("unavailable memory for object %d\n",current);
		exit(EXIT_FAILURE);
	}
	if ((allocators[current])[index].top_elem == (allocators[current])[index].size){
		index++;
		goto redo;
	}
	chunk_address = (allocators[current])[index].addresses[(allocators[current])[index].top_elem];
	(allocators[current])[index].top_elem++;
	AUDIT printf("returning address %p to object %d\n",chunk_address,current);
	return chunk_address;
}

void __wrap_free(void *ptr){
	int current;
	int index;
	current = get_current();
	AUDIT printf("freeing address %p for object %d\n",ptr,current);
	if (ptr < base[current] || ptr >= (base[current] + MAX_MEMORY)){
		printf("bad address for free by object %d\n",current);
		exit(EXIT_FAILURE);
	}
	index = (int)((double)((ptr - base[current])>>12) / (double)(SEGMENT_PAGES));
	AUDIT printf("wrapping free for object %d - index is %d\n",current,index);
	(allocators[current])[index].addresses[--(allocators[current])[index].top_elem] = ptr;
	if ((allocators[current])[index].top_elem < 0){
		printf("allocator corruption on free by object %d\n",current);
		exit(EXIT_FAILURE);
	}
	
}
