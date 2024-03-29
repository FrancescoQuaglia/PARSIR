
#ifndef __QUEUE
#define QUEUE

#include <pthread.h>
#include <setup.h>

#define NUM_SLOTS (2000)
#define SLOT_LEN (LOOKAHEAD)
#define QUEUE_TIME_INTERVAL (SLOT_LEN * NUM_SLOTS)
typedef struct _queue_elem{
	int destination;
	double timestamp;
	struct _queue_elem * next;
	struct _queue_elem * prev;
} queue_elem;

typedef struct _slot{
	queue_elem  head;
	queue_elem  tail;
} slot;

typedef struct _fallbacks_lot{
	queue_elem*  head;
	queue_elem*  tail;
} fallback_slot;

typedef union _lock_buffer{
	pthread_spinlock_t lock;
	char buff[64];
} __attribute__((packed)) lock_buffer;

int queue_init(void);
void whoami(unsigned);
int queue_insert(queue_elem * elem);
queue_elem * queue_extract(void);
int barrier(void);

#define AUDIT if(0)

#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})

#endif
