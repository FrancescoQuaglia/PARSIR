#define _GNU_SOURCE
#include <pthread.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <queue.h>
#include <memory.h>

#include <sys/time.h>
#include <sys/resource.h>


#define MAX_EVENT_SIZE  (512)


typedef struct _event_buffer{
	int destination;
	double timestamp;
	int event_type;
	char payload[MAX_EVENT_SIZE];
	int event_size;
} event_buffer;


typedef struct _event{
	event_buffer e;
	queue_elem q;
} event;

typedef struct _thread_startup{
	long vTID;
	int minID;
	int maxID;
	int cpuID;//the thread need to pin to this CPU ID in order 
		  //to correctly stay on the NUMA node ID
	int numaNodeID;//this is the NUMA node id
		       //where the thread will reside
	int numaNodePerObjects; //this is the NUIMA node where
				//the objects started up by this thread will reside
	//the below fields are for reaching the stuff needed
	//for NUMA oriented workload distribution
	int * c;
	int * min;
	int * max;
} thread_startup;

__thread thread_startup * thread_startup_data;

thread_startup startup_info[THREADS];
volatile int processed_events[THREADS];
#define PERIOD (5)//this is setup in seconds
#define DURATION (60)//this is setup in seconds


__thread int current = -1;
__thread int NUMAnode = -1;
__thread int numaNodePerObjects = 0;


inline int get_current(void){
	return current;
}

inline int get_NUMAnode(void){
	return numaNodePerObjects;//NUMAnode;
	//return NUMAnode;
}


inline int * getcounter(void){ 
	return  thread_startup_data->c;
}
inline int * getmin(void){ 
	return  thread_startup_data->min;
}
inline int * getmax(void){ 
	return  thread_startup_data->max;
}

//This is the PARSIR worker thread
void * thread(void* me){

	pthread_t thread;//for self recognition
	cpu_set_t cpuset;
	int i;
	int destination;
	double timestamp;
	int event_type;
	int size;
	int ret;
	unsigned long aux = ((thread_startup*)me)->vTID;
	int minID = ((thread_startup*)me)->minID;
	int maxID = ((thread_startup*)me)->maxID;
	int cpuID = ((thread_startup*)me)->cpuID; 
	int numaNodeID = ((thread_startup*)me)->numaNodeID; 
	event_buffer * the_event;
	//this is only used for awoiding compile-time warning

	thread_startup_data = (thread_startup*)me;

	thread = pthread_self();
	numaNodePerObjects = ((thread_startup*)me)->numaNodePerObjects;

	//this setus up a TLS variable usable in any other function
  	//it is exploited by PARSIR for the memory startup of the objects
	NUMAnode = numaNodeID;

	printf("PARSIR worker thread %ld started - CPU to pin on is %d - NUMA node is %d\n",aux,cpuID,numaNodeID);

	CPU_SET(cpuID, &cpuset);
	if (pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset) != 0){
		printf("PARSIR worker thread %ld error while pinning to CPU %d\n",aux,cpuID);
		fflush(stdout);
		exit(EXIT_FAILURE);
	}

	whoami((unsigned)aux);	//tell the queue layer your identity 
				//this is used for TLS setup stuff


	AUDIT printf("PARSIR worker thread %ld received min %d and max %d\n",aux,minID,maxID);
	for (; minID <= maxID; minID++){
		AUDIT printf("thread %ld - starting up object %d\n",aux, minID);
		fflush(stdout);
		current = minID;
		object_allocator_setup();//you cannot init any object if its chunk allocator is not setup
		AUDIT printf("thread %ld - setting up object %d\n",aux,current);
		ProcessEvent(minID, STARTUP_TIME, INIT, NULL, 0, NULL);
		current = -1;
	}	
	//the init phase is over
	//we need to wait each other for correct 
	//management of the lookahead epoch in
	//the event queue
	barrier();

	//this is the buffer used to extract events from the queue
	//a single buffer is sufficient for iterative processing of
	//the extracted events
	the_event = malloc(sizeof(event));
	if (!the_event){
		printf("test queue error 1 - event allocation failure\n");
		fflush(stdout);
		exit(EXIT_FAILURE);
	}

	do {
		ret = GetEvent(&(the_event -> destination), &(the_event->timestamp), &(the_event -> event_type),NULL, &(the_event->event_size) ); 
		if(!ret){
			AUDIT printf("thread %ld got event: %e -  %d - %d\n",
			aux,
			the_event -> timestamp,
			the_event -> destination,
			the_event -> event_type);
			current = the_event -> destination;
			ProcessEvent(the_event -> destination, the_event->timestamp, the_event -> event_type, the_event->payload, the_event->event_size, NULL);
			current = -1;
		}
		processed_events[aux]++;
	} while (ret == 0);

	free(the_event);

	printf("thread %ld - no more events to process\n",aux);

	return NULL;

}

#define LINE 1024
int hw = 0;
int CPUs = 0;
int NUMA_NODES = 0;
int cpu_id_per_numa_node[MAX_NUMA_NODES][MAX_CPUS_PER_NODE];
int cpus_per_numa_node[MAX_NUMA_NODES] = { [0 ... (MAX_NUMA_NODES-1)] -1};
int objects_per_numa_node[MAX_NUMA_NODES] = { [0 ... (MAX_NUMA_NODES-1)] 0 };

inline int get_totNUMAnodes(void){
	return NUMA_NODES;
}

//the below stuff is for NUMA workload distribution
int c[MAX_NUMA_NODES] = { [0 ... MAX_NUMA_NODES-1] 0};
int min[MAX_NUMA_NODES] = { [0 ... MAX_NUMA_NODES-1] 0};
int max[MAX_NUMA_NODES] = { [0 ... MAX_NUMA_NODES-1] 0};

//this function attemts to setup data based on the HW level
//configuration of the machine
//in case the HW level configuration file is not available in the 
//$(INST_DIR)/setup-data directory the function simply returns
//and the engine will execute accodig to the default setup
//that corresponds to single NUMA node
void get_hw_config(void){
	FILE * config = NULL;
	char buff[LINE];
	int i, j, z;
	char * temp;
	int lower, upper, counter;
	int ret;
	char * p;

	printf("PARSIR started with %d threads\n",THREADS);

	//get the total number of CPUs in the machine
	CPUs = sysconf(_SC_NPROCESSORS_ONLN);

	if(THREADS > CPUs){
		printf("Too many threads have been requsted (the available CPUs are %d)\n",CPUs);
		exit(EXIT_FAILURE);
	}

	config = fopen("../setup-data/hw.txt","r");
	if(!config){
		printf("HW level config file not available - running with default config\n");
		goto default_config;
	}
	printf("HW level config file available\n");
	hw = 1;

	for (i=0; i < 3; i++) ret = fscanf(config,"%s",buff);
	NUMA_NODES = strtol(buff,NULL,10);
	printf("PARSIR running with %d NUMA node(s)\n",NUMA_NODES);
	for(j = 0 ; j < NUMA_NODES; j++){
		for (i=0; i < 3; i++) ret = fscanf(config,"%s",buff);
		p = fgets(buff,LINE,config);
		printf("%s",buff);
		fflush(stdout);
		temp = strtok(buff,",\n");
		lower = upper = -1;
		counter = 0;
		while(temp && *temp){
			for(z = 0; z < strlen(temp); z++){
				if (temp[z] == '-'){
					lower = strtol(temp,NULL,10);
					upper = strtol(&temp[z + 1],NULL,10);
					break;
				}
			}
			if (lower != -1){
				for(z = 0; z <= (upper - lower); z++){
					cpu_id_per_numa_node[j][counter] = lower + strtol(temp,NULL,10) + z;
					counter++;
					printf("node %d has CPU %ld\n",j,lower+strtol(temp,NULL,10)+z);
				}
			}
			else{
				cpu_id_per_numa_node[j][counter] = strtol(temp,NULL,10);
				counter++;
			}
			temp = strtok(NULL,",\n");
		}
		cpus_per_numa_node[j] = counter;
		printf("assigned %d CPUs to the entry of node %d\n",counter,j);
		printf("ids are: ");
		for(z = 0; z < counter; z++) printf("%d ",cpu_id_per_numa_node[j][z]);
		printf("\n");
	}

	printf("total number of available CPUs is %d\n",CPUs);

	return;

default_config:
	NUMA_NODES = 1;
	cpus_per_numa_node[0] = CPUs;
	for (i = 0; i < CPUs; i++) cpu_id_per_numa_node[0][i] = i;
	c[0] = OBJECTS;
	min[0] = 0;
	max[0] = OBJECTS - 1;
	return;
}


int main(int argc, char** argv){

	long i, j;
	event * the_event;
	queue_elem * q;
	pthread_t tid;
	float ratio;
	float NUMAratio;
	int current_min;
	int corrector = 0;
	int NUMAcorrector = 0;
	int numaNodeRound = 0;//this is for assigning a thread to a NUMA node
	int numaNodeInit = 0;//this is for setting up objects to NUMA node(s)
			     //starting form the NUMA node with id set to 0
	unsigned long total;
	int stable = 0;

	//this function uses the lscpu command to take information
	//on the hardware level configuration
	//if this command is not available a default config is adopted 
	get_hw_config();

	queue_init();
	allocators_base_init();

	ratio = (float)OBJECTS / (float)THREADS;
	if ( ratio - (int) ratio > 0) corrector = 1;
	printf("engine - ratio is %f\n",ratio);

	NUMAratio = (float)OBJECTS / (float)NUMA_NODES;
	if ( NUMAratio - (int) NUMAratio > 0) NUMAcorrector = 1;
	printf("engine - NUMA ratio is %f\n",NUMAratio);

	current_min = 0;
	for (i = 0; i < THREADS; i++){
		startup_info[i].vTID = i;
		startup_info[i].minID = current_min;
		startup_info[i].maxID = current_min + (int)ratio + corrector - 1;
		//this check enables threads to discard setup of objects with maxID < minID
		if (startup_info[i].maxID > OBJECTS - 1) startup_info[i].maxID = OBJECTS - 1;
		current_min += (int)ratio + corrector;
		if (NUMA_NODES == 1){
			startup_info[i].cpuID = cpu_id_per_numa_node[0][i];;
			startup_info[i].numaNodeID = 1;
		}
		else{
retry:
			for(j = 0; j < cpus_per_numa_node[numaNodeRound]; j++){
				if (cpu_id_per_numa_node[numaNodeRound][j] != -1){
					startup_info[i].cpuID = cpu_id_per_numa_node[numaNodeRound][j];
					startup_info[i].numaNodeID = numaNodeRound;
					//eliminating the assigned ID
					cpu_id_per_numa_node[numaNodeRound][j] = -1;
				  	numaNodeRound = (numaNodeRound+1) % NUMA_NODES;	
					goto done;
				}

			}	
			numaNodeRound = (numaNodeRound+1) % NUMA_NODES;	
			goto retry;
		}
done:

		//setup of basic stuff 
		startup_info[i].c  = c;
		startup_info[i].min  = min;
		startup_info[i].max  = max;

		//now we setup the NUMA node id where the objects 
		//started up by this new thread will reside
		//this neews to occurr oly if maxID >= minID
		//otherwise the NUMA node id is a do not care
		startup_info[i].numaNodePerObjects = 0;
		if(NUMA_NODES == 1) goto NUMAdone;

		//current NUMA node is used for objects startup
		startup_info[i].numaNodePerObjects = numaNodeInit;


		//now we check if the NUMA node for startup needs to be updated
		//and we also update the number of hosted objects on this NUMA node
		if(startup_info[i].maxID >= startup_info[i].minID) {
	
			c[numaNodeInit] += (startup_info[i].maxID - startup_info[i].minID + 1);
			max[numaNodeInit] = startup_info[i].maxID;
			if (((int)((double)startup_info[i].maxID / (double)NUMAratio)) > numaNodeInit){
				printf("update of the NUMA node for object startup is requested (i is %ld)\n",i);
				numaNodeInit++;
				min[numaNodeInit] = startup_info[i].maxID + 1;
			};
		}
NUMAdone:	;
		
	}

	for(i = 0 ; i < THREADS; i++){
		pthread_create(&tid, 0x0, thread, (void*)&(startup_info[i]));
	}

	for(i = 0 ; i < NUMA_NODES; i++){
		printf("objects assigned to NUMA node %ld are %d min is %d - max is %d\n",i,c[i],min[i],max[i]);
	}

	pause();
	
}

