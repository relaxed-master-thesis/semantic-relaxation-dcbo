#ifndef EXTERNAL_COUNT_WRAPPER_H
#define EXTERNAL_COUNT_WRAPPER_H

#include "partial-ms.h"

// Define generics for d-balanced-queue
#define PARTIAL_T                   wrapped_queue_t
#define PARTIAL_ENQUEUE(q, k, v)    wrapped_enqueue(q, k, v)
#define PARTIAL_DEQUEUE(q)          wrapped_dequeue(q)
#define INIT_PARTIAL(q,n)           init_wrapped_queue(q)
#define PARTIAL_LENGTH(q)           wrapped_queue_size(q)
#define PARTIAL_TAIL_VERSION(q)     wrapped_tail_version(q)
#define PARTIAL_ENQ_COUNT(q)        wrapped_enq_count(q)
#define PARTIAL_DEQ_COUNT(q)        wrapped_deq_count(q)
#define EMPTY				        ((sval_t)0)



typedef struct counter_wrapper_queue {
    ALIGNED(CACHE_LINE_SIZE) volatile uint64_t enq_count;
    ALIGNED(CACHE_LINE_SIZE) volatile uint64_t deq_count;
    ALIGNED(CACHE_LINE_SIZE) ms_queue_t partial;
} wrapped_queue_t;

/* Exported functions */
int wrapped_enqueue(wrapped_queue_t *queue, skey_t key, sval_t val);
sval_t wrapped_dequeue(wrapped_queue_t *queue);
void init_wrapped_queue(wrapped_queue_t *queue);
size_t wrapped_queue_size(wrapped_queue_t *queue);
int wrapped_tail_version(wrapped_queue_t *queue);
uint64_t wrapped_enq_count(wrapped_queue_t *queue);
uint64_t wrapped_deq_count(wrapped_queue_t *queue);

#endif