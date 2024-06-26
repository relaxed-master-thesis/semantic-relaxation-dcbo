#ifndef PARTIAL_WFQUEUE_H
#define PARTIAL_WFQUEUE_H

#include <pthread.h>
#include "common.h"
#include "ssmem.h"
// Old include
#include "align.h"

#define DS_ADD(h,k,v)       enqueue_wrap(h, (void*) v)
#define DS_REMOVE(h)        dequeue_wrap(h)
#define DS_SIZE(s)          wfqueue_length_heuristic(s)
#define DS_NEW(n,i)         wfqueue_create(n,i)
#define DS_REGISTER(q,i)    wfqueue_register(q,i)

#define DS_HANDLE           handle_t*
#define DS_TYPE             queue_t
#define DS_NODE             sval_t

#define INTERNAL_EMPTY ((void *) 0)

#ifndef WFQUEUE_NODE_SIZE
#define WFQUEUE_NODE_SIZE ((1 << 10) - 2)
#endif

struct _enq_t {
  long volatile id;
  void * volatile val;
} CACHE_ALIGNED;

struct _deq_t {
  long volatile id;
  long volatile idx;
} CACHE_ALIGNED;

struct _cell_t {
  void * volatile val;
  struct _enq_t * volatile enq;
  struct _deq_t * volatile deq;
  // void * pad[5];
};

struct _node_t {
  struct _node_t * volatile next CACHE_ALIGNED;
  long id CACHE_ALIGNED;
  struct _cell_t cells[WFQUEUE_NODE_SIZE] CACHE_ALIGNED;
};

typedef struct ALIGNED(CACHE_LINE_SIZE) {
  /**
   * Index of the next position for enqueue.
   */
  volatile long Ei ALIGNED(CACHE_LINE_SIZE);

  /**
   * Index of the next position for dequeue.
   */
  volatile long Di ALIGNED(CACHE_LINE_SIZE);

  /**
   * Index of the head of the queue.
   */
  volatile long Hi ALIGNED(CACHE_LINE_SIZE);

  /**
   * Pointer to the head node of the queue.
   */
  struct _node_t * volatile Hp;

  /**
   * Pointer to the last registered handle for setup.
  */
  struct _handle_t * volatile _tail_handle;

  /**
   * Number of processors.
   */
  long nprocs;
#ifdef RECORD
  long slowenq;
  long slowdeq;
  long fastenq;
  long fastdeq;
  long empty;
#endif
} queue_t;

typedef struct _handle_t {
  /**
   * Pointer to the next handle.
   */
  struct _handle_t * next;

  /**
   * Hazard pointer.
   */
  //struct _node_t * volatile Hp;
  unsigned long volatile hzd_node_id;

  /**
   * Pointer to the node for enqueue.
   */
  struct _node_t * volatile Ep;
  unsigned long enq_node_id;

  /**
   * Pointer to the node for dequeue.
   */
  struct _node_t * volatile Dp;
  unsigned long deq_node_id;

  /**
   * Enqueue request.
   */
  struct _enq_t Er CACHE_ALIGNED;

  /**
   * Dequeue request.
   */
  struct _deq_t Dr CACHE_ALIGNED;

  /**
   * Handle of the next enqueuer to help.
   */
  struct _handle_t * Eh CACHE_ALIGNED;

  long Ei;

  /**
   * Handle of the next dequeuer to help.
   */
  struct _handle_t * Dh;

  /**
   * Pointer to a spare node to use, to speedup adding a new node.
   */
  struct _node_t * spare CACHE_ALIGNED;

  /**
   * Count the delay rounds of helping another dequeuer.
   */
  int delay;

  /**
   * Pointer to the associated queue
  */
  queue_t* queue;

#ifdef RECORD
  long slowenq;
  long slowdeq;
  long fastenq;
  long fastdeq;
  long empty;
#endif
} handle_t;


extern __thread ssmem_allocator_t* alloc;

// Expose functions
int enqueue_wrap(handle_t *th, void *v);
sval_t dequeue_wrap(handle_t *th);
queue_t* wfqueue_create(int nprocs, int thread_id);
handle_t* wfqueue_register(queue_t *q, int id);
uint64_t wfqueue_enq_count(queue_t *q);
uint64_t wfqueue_deq_count(queue_t *q);
uint64_t wfqueue_length_heuristic(queue_t *q);

#endif /* end of include guard: PARTIAL_WFQUEUE_H */

