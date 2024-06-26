#ifndef D_BALANCED_MS_H
#define D_BALANCED_MS_H

#include <assert.h>
#include <getopt.h>
#include <limits.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <stdint.h>
#include "common.h"

#include "lock_if.h"
#include "ssmem.h"
#include "utils.h"

#ifdef RELAXATION_TIMER_ANALYSIS
#include "relaxation_analysis_timestamps.h"
#elif RELAXATION_ANALYSIS
#include "relaxation_analysis_queue.h"
#endif

// Define generics for d-balanced-queue
#define PARTIAL_T                   ms_queue_t
#define PARTIAL_ENQUEUE(q, k, v)    ms_enqueue(q, k, v)
#define PARTIAL_DEQUEUE(q)          ms_dequeue(q)
#define INIT_PARTIAL(q,i)           init_ms_queue(q)
#define PARTIAL_LENGTH(q)           ms_queue_size(q)
#define PARTIAL_TAIL_VERSION(q)		ms_enq_count(q)
#define PARTIAL_ENQ_COUNT(q)        ms_enq_count(q)
#define PARTIAL_DEQ_COUNT(q)        ms_deq_count(q)
#define EMPTY						((sval_t)0)


/* Type definitions */
typedef struct mqueue_node
{
	skey_t key;
	sval_t val;
	struct mqueue_node* volatile next;
	uint8_t padding[CACHE_LINE_SIZE - sizeof(skey_t) - sizeof(sval_t) - sizeof(struct mqueue_node*)];
} node_t;

typedef struct file_descriptor
{
	node_t* node;
	uint64_t count;
} descriptor_t;

typedef ALIGNED(CACHE_LINE_SIZE) struct array_index
{
    volatile descriptor_t head;
    volatile descriptor_t tail;
	uint8_t padding[CACHE_LINE_SIZE - 2*sizeof(descriptor_t)];
} ms_queue_t;


/*Global variables*/


/*Thread local variables*/
extern __thread ssmem_allocator_t* alloc;
extern __thread int thread_id;

extern __thread unsigned long my_put_cas_fail_count;
extern __thread unsigned long my_get_cas_fail_count;
extern __thread unsigned long my_null_count;
extern __thread unsigned long my_hop_count;
extern __thread unsigned long my_slide_count;

/* Interfaces */
int ms_enqueue(ms_queue_t *set, skey_t key, sval_t val);
sval_t ms_dequeue(ms_queue_t *q);
void init_ms_queue(ms_queue_t *q);
size_t ms_queue_size(ms_queue_t *set);
uint64_t ms_enq_count(ms_queue_t *set);
uint64_t ms_deq_count(ms_queue_t *set);

#endif // D_BALANCED_MS_H
