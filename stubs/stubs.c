#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <queue.h>
#include <unistd.h>

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


int ScheduleNewEvent(int destination, double timestamp, int event_type, char* body, int size){

	event * p = NULL;

	if(size > MAX_EVENT_SIZE) {
		printf("event size too large\n");
		fflush(stdout);
		return -1;
	}

	if(size < 0) {
		printf("negative event size\n");
		fflush(stdout);
		return -1;
	}

	if((size > 0) && (body == NULL)) {
		printf("invalid null address for the event body\n");
		fflush(stdout);
		return -1;
	}
	
	p = malloc(sizeof(event));//allocating the actual storage for the event
	if (!p) {
		printf("event allocation failure\n");
		fflush(stdout);
		return -1;
	}

	p->e.destination = destination;
	p->e.timestamp = timestamp;
	p->e.event_type = event_type;
	p->e.event_size = size;
	memcpy(&(p->e.payload),body,size);

	p->q.destination = destination;
	p->q.timestamp = timestamp;

	return queue_insert(&(p->q));

}

int GetEvent(int *destination, double *timestamp, int *event_type, char* body, int * size){
	event_buffer * e;
	queue_elem * q;

	q = queue_extract();
	if (q == NULL){
		printf("empty event queue\n");
		return -1;
	}
	e = (event_buffer*)container_of(q, event, q);

	*destination = e->destination;
	*timestamp = e->timestamp;
	*event_type = e->event_type;;
	*size = e->event_size;
	memcpy(body, e->payload, e->event_size);

	free(e);

	return 0;
}

