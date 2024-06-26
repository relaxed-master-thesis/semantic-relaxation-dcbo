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


#define DS_ADD(s,k,v)       ms_enqueue(s,k,v)
#define DS_REMOVE(s)        ms_dequeue(s, NULL)
#define DS_SIZE(s)          ms_queue_size(s)
#define DS_NEW(i)           create_ms_queue(i)
#define DS_REGISTER(s,i)    queue_register(s,i)

#define DS_TYPE             ms_queue_t
#define DS_HANDLE           ms_queue_t*
#define DS_NODE             sval_t

// Define generics for d-balanced-queue
#define PARTIAL_T                   ms_queue_t
#define PARTIAL_ENQUEUE(q, k, v)    ms_enqueue(q, k, v)
#define PARTIAL_DEQUEUE(q, c)       ms_dequeue(q, c)
#define INIT_PARTIAL(q)             init_ms_queue(q)
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
	// uint8_t padding[CACHE_LINE_SIZE - sizeof(skey_t) - sizeof(sval_t) - sizeof(struct mqueue_node*)];
} node_t;

typedef struct file_descriptor
{
	node_t* node;
	uint64_t count;
} descriptor_t;

typedef ALIGNED(CACHE_LINE_SIZE) struct array_index
{
    volatile descriptor_t head;
	uint8_t padding1[CACHE_LINE_SIZE - sizeof(descriptor_t)];
    volatile descriptor_t tail;
	uint8_t padding2[CACHE_LINE_SIZE - sizeof(descriptor_t)];
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
sval_t ms_dequeue(ms_queue_t *q, uint64_t *double_collect_count);
size_t ms_queue_size(ms_queue_t *set);
uint64_t ms_enq_count(ms_queue_t *set);
uint64_t ms_deq_count(ms_queue_t *set);
ms_queue_t *create_ms_queue(int thread_id);
ms_queue_t* queue_register(ms_queue_t* set, int thread_id);

#endif // D_BALANCED_MS_H
