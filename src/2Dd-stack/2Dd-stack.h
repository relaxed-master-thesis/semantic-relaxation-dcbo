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

 /* ################################################################### *
	* Definition of macros: per data structure
* ################################################################### */

#define DS_ADD(s,k,v)       push(s,k,v)
#define DS_REMOVE(s)        pop(s)
#define DS_SIZE(s)          stack_size(s)
#define DS_NEW(n,w,d,m,k)   create_stack(n,w,d,m,k)
#define DS_REGISTER(s,i)    register_stack(s,i)

#define DS_TYPE             mstack_t
#define DS_HANDLE           mstack_t*
#define DS_NODE             node_t

/* Type definitions */
typedef struct mstack_node
{
	skey_t key;
	sval_t val;
	struct mstack_node* next;
	uint8_t padding[CACHE_LINE_SIZE - sizeof(skey_t) - sizeof(sval_t) - sizeof(struct mstack_node*)];
} node_t;

typedef struct file_descriptor
{
	node_t* node;
	uint32_t put_count;
	uint32_t get_count;
} descriptor_t;
typedef ALIGNED(CACHE_LINE_SIZE) struct array_index
{
	volatile descriptor_t descriptor;
	uint8_t padding[CACHE_LINE_SIZE - sizeof(descriptor_t)];
} index_t;

typedef ALIGNED(CACHE_LINE_SIZE) struct mstack_file
{
	index_t *get_array;
	index_t *put_array;
	uint64_t random_hops;
	uint64_t width;
	uint64_t depth;
	uint64_t relaxation_bound;
	uint8_t k_mode;
	uint8_t padding[CACHE_LINE_SIZE - sizeof(uint8_t) - (2 * sizeof(index_t*)) - (sizeof(int64_t)*4)];
} mstack_t;

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
int push(mstack_t *set, skey_t key, sval_t val);
sval_t pop(mstack_t *set);
node_t* create_node(skey_t key, sval_t val, node_t* next);
mstack_t* create_stack(size_t num_threads, uint64_t width, uint64_t depth, uint8_t k_mode, uint64_t relaxation_bound);
mstack_t* register_stack(mstack_t *set, int thread_id);
size_t stack_size(mstack_t *set);
int floor_log_2(unsigned int n);