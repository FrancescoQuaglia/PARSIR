
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "application.h"
#include "setup.h"

lp_state_type* states[OBJECTS];

#define state states[me]


//stuff for managing the allocation/deallocation
//of chunks which the state of the simulation objects
void add1(datatype1 ** head, datatype1 * buff){
		buff->p = *head;
		*head = buff;	
}
void dell1(datatype1 ** head){
		datatype1 * aux;
		if(*head == NULL) return;
		aux = *head;
		*head = (*head)->p;
		free((void*)aux);
}

void add2(datatype2 ** head, datatype2 * buff){
		buff->p = *head;
		*head = buff;	
}
void dell2(datatype2 ** head){
		datatype2 * aux;
		if(*head == NULL) return;
		aux = *head;
		*head = (*head)->p;
		free((void*)aux);
}

//stuff for managing the actual memory access to the state content
void read1(datatype1 ** p){
	char buff[SIZE1];
	int  count = 0;
	while(*p && count < P){
		memcpy((void*)buff,(void*)p,SIZE1);
		count++;
		p = (*p)->p;
	}	
}

void read2(datatype2 ** p){
	char buff[SIZE2];
	int  count = 0;
	while(*p && count < P){
		memcpy((void*)buff,(void*)p,SIZE2);
		count++;
		p = (*p)->p;
	}	
}

//callback function for processing an event at an object
void ProcessEvent(unsigned int me, double now, int event_type, void *event_content, unsigned int size, void *ptr) {

	double timestamp;
	uint32_t *s1, *s2;
	unsigned int dest;
	event_content = event_content;
	int res;
	int i;

	//just bypassing compile time indications
	//on unused parameters
	ptr = ptr;
	size = size;

//	printf("object %d - executing event %d at time %f\n", me, event_type, now);

	switch(event_type) {

		case INIT:

			// Initialize the LP's state
			state  = (lp_state_type *) malloc(sizeof(lp_state_type));
			if (state == NULL){
				printf("out of memory at startup\n");
				exit(EXIT_FAILURE);
			}
			state->event_count = 0;

			s1 = &(state->seed1);
			s2 = &(state->seed2);
			*s1 = (0x01 * me) + 1;
			*s2 = *s1 ^ *s1;

			state->p1 = NULL;
			state->p2 = NULL;

			for (i=0;i<M;i++){

				//setup of the initial events 
				//timestamp = now +  0.1 * Expent(TA,s1,s2);
				timestamp = 0.01 * Expent(TA,s1,s2);
				dest = me;
				ScheduleNewEvent(dest, timestamp, NORMAL, NULL, 0);
//				printf("object %d - scheduled event %d - timestamp is %e - destination is %d\n ", me, NORMAL, timestamp, dest);
			}

			//setup of dynamic memory lists 
			//in the object state
			for (i=0;i<CHUNKS_IN_LIST;i++){
				add1(&(state->p1),(datatype1*)malloc(SIZE1));
			}
			for (i=0;i<CHUNKS_IN_LIST;i++){
				add2(&(state->p2),(datatype2*)malloc(SIZE2));
			}

			break;


		case NORMAL:

			res = (state->event_count++) % 1000;
			//res = res;
			if (!res) printf("object %d - count of events is %d\n",me,state->event_count);

			s1 = &(state->seed1);
			s2 = &(state->seed2);

			//schedule a new event in the model
			//timestamp = now + LOOKAHEAD + Expent(TA,s1,s2);
			timestamp = now + Expent(TA,s1,s2);
			//if (timestamp <= now + LOOKAHEAD) timestamp = now + LOOKAHEAD + FLT_EPSILON;
			if (timestamp <= now + LOOKAHEAD) timestamp = now + LOOKAHEAD + Expent(TA,s1,s2);
			dest = (int) ((double)(OBJECTS) * Random(s1,s2));
			//printf("destination is %d\n",dest);

			ScheduleNewEvent(dest, timestamp, NORMAL, NULL, 0);
			//printf("object %d - scheduled event %d - timestamp is %e - destination is %d\n ",me, NORMAL, timestamp, dest);

			for(i = 0; i < REALLOCATION; i++){
				dell1(&(state)->p1);
				dell2(&(state)->p2);
			}	
			for (i=0;i<REALLOCATION;i++){
				add1(&(state->p1),(datatype1*)malloc(SIZE1));
			}
			for (i=0;i<REALLOCATION;i++){
				add2(&(state->p2),(datatype2*)malloc(SIZE2));
			}

			read1(&(state->p1));
			read2(&(state->p2));
			
			break;

		default:
			printf("phold: unknown event type! (me = %d - event type = %d)\n", me, event_type);
			exit(EXIT_FAILURE);

	}

}
