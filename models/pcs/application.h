#include <stdint.h>

#include "setup.h"
#include "memory.h"


int getindex(void);

#define TOPOLOGY_HEXAGON	1000
#define TOPOLOGY_SQUARE		1001
#define TOPOLOGY_MESH		1002
#define TOPOLOGY_STAR		1003
#define TOPOLOGY_RING		1004
#define TOPOLOGY_BIDRING	1005
unsigned int FindReceiver(int, int, uint32_t *, uint32_t *);



/* DISTRIBUZIONI TIMESTAMP */
#define UNIFORM		0
#define EXPONENTIAL	1
#define DISTRIBUTION	1


#define CHECK_FADING_TIME	10
#define COMPLETE_CALLS		5000
#ifndef TA
#define TA			0.4 
#endif
#define TA_DURATION		120

#ifndef CHANNELS_PER_CELL
#define CHANNELS_PER_CELL	5000
#endif

#define INITIAL_CALLS		(int)((double)TA_DURATION / ((double)(TA))) //channels busy at simulaitn startup

#ifndef TA_CHANGE
#define TA_CHANGE		300.0
#endif

#define	CELL_CHANGE_DISTRIBUTION	EXPONENTIAL
#define DURATION_DISTRIBUTION		EXPONENTIAL

#define HANDOFF_SHIFT 0.000001

typedef double simtime_t;
typedef enum { false, true } bool;

/* Channel states */
#define CHAN_BUSY	1
#define CHAN_FREE	0

/* EVENT TYPES - PCS */
#define START_CALL	20
#define END_CALL	21
#define HANDOFF_LEAVE	30
#define HANDOFF_RECV	31
#define HANDOFF_IN	32
#define FADING_RECHECK	40

#define FADING_RECHECK_FREQUENCY	300	// Every 5 Minutes

#define MSK 0x1
#define SET_CHANNEL_BIT(B,K) ( B |= (MSK << K) )
#define RESET_CHANNEL_BIT(B,K) ( B &= ~(MSK << K) )
#define CHECK_CHANNEL_BIT(B,K) ( B & (MSK << K) )

#define BITS (sizeof(int) * 8)

#define CHECK_CHANNEL(P,I) ( CHECK_CHANNEL_BIT(						\
	((unsigned int*)(((lp_state_type*)P)->channel_state))[(int)((int)I / BITS)],	\
	((int)I % BITS)) )

#ifdef NUMA_UBIQUITOUS
#define SET_CHANNEL(P, I) do { \
    SET_CHANNEL_BIT( \
        ((unsigned int*)(((lp_state_type*)P)->channel_state))[(I / BITS)], \
        (I % BITS) \
    ); \
    SET_MEMORY( \
        &((unsigned int*)(((lp_state_type*)P)->channel_state))[(I / BITS)], \
        ((unsigned int*)(((lp_state_type*)P)->channel_state))[(I / BITS)] \
    ); \
} while(0)

#define RESET_CHANNEL(P,I) do { \
    RESET_CHANNEL_BIT( \
	((unsigned int*)(((lp_state_type*)P)->channel_state))[(int)((int)I / BITS)],\
	((int)I % BITS) \
    ); \
    SET_MEMORY( \
        &((unsigned int*)(((lp_state_type*)P)->channel_state))[(I / BITS)], \
        ((unsigned int*)(((lp_state_type*)P)->channel_state))[(I / BITS)] \
    ); \
} while(0)

#else

#define SET_CHANNEL(P,I) ( SET_CHANNEL_BIT(                                             \
        ((unsigned int*)(((lp_state_type*)P)->channel_state))[(int)((int)I / BITS)],    \
        ((int)I % BITS)) )
#define RESET_CHANNEL(P,I) ( RESET_CHANNEL_BIT(                                         \
        ((unsigned int*)(((lp_state_type*)P)->channel_state))[(int)((int)I / BITS)],    \
        ((int)I % BITS)) )

#endif

// Message exchanged among LPs
typedef struct _event_content_type {
	int cell; // The destination cell of an event
	unsigned int from; // The sender of the event (in case of HANDOFF_RECV)
	simtime_t sent_at; // Simulation time at which the call was handed off
	int channel; // Channel to be freed in case of END_CALL
	simtime_t   call_term_time; // Termination time of the call (used mainly in HANDOFF_RECV)
	int *dummy;
} event_content_type;

#define CROSS_PATH_GAIN		0.00000000000005
#define PATH_GAIN		0.0000000001
#define MIN_POWER		3
#define MAX_POWER		3000
#define SIR_AIM			10

typedef struct _sir_data_per_cell{
    double fading; // Fading of the call
    double power; // Power allocated to the call
} sir_data_per_cell;

typedef struct _channel{
	int channel_id; // Number of the channel
	sir_data_per_cell *sir_data; // Signal/Interference Ratio data
	struct _channel *next;
	struct _channel *prev;
} channel;


typedef struct _lp_state_type{
	int cell_id;
	int ecs_count;
	uint32_t seed1;//seed passed in input to randomization functions
        uint32_t seed2;//seed passed in input to randomization functions


	unsigned int channel_counter; // How many channels are currently free
	unsigned int arriving_calls; // How many calls have been delivered within this cell
	unsigned int complete_calls; // Number of calls which were completed within this cell
	unsigned int blocked_on_setup; // Number of calls blocked due to lack of free channels
	unsigned int blocked_on_handoff; // Number of calls blocked due to lack of free channels in HANDOFF_RECV
	unsigned int leaving_handoffs; // How many calls were diverted to a different cell
	unsigned int arriving_handoffs; // How many calls were received from other cells
	unsigned int cont_no_sir_aim; // Used for fading recheck
	unsigned int executed_events; // Total number of events

	simtime_t lvt; // Last executed event was at this simulation time

	double ta; // Current call interarrival frequency for this cell
	double ref_ta; // Initial call interarrival frequency (same for all cells)
	double ta_duration; // Average duration of a call
	double ta_change; // Average time after which a call is diverted to another cell

	int channels_per_cell; // Total channels in this cell
	int total_calls;

	bool check_fading; // Is the model set up to periodically recompute the fading of all ongoing calls?
	bool fading_recheck;
	bool variable_ta; // Should the call interarrival frequency change depending on the current time?

	unsigned int *channel_state;
	struct _channel *channels;
	int dummy;
	bool dummy_flag;
} lp_state_type;


double recompute_ta(double ref_ta, simtime_t now);
double generate_cross_path_gain(uint32_t *, uint32_t *);
double generate_path_gain(uint32_t *, uint32_t *);
void deallocation(unsigned int lp, lp_state_type *state, int channel, simtime_t);
int allocation(lp_state_type *, uint32_t *, uint32_t *);
void fading_recheck(lp_state_type *, uint32_t *, uint32_t *);
double my_pow(double, int);


extern int channels_per_cell;
