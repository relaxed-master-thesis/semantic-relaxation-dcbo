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
#include "lateral_stack.h"
#include "types.h"

#ifdef RELAXATION_ANALYSIS
#include "relaxation_analysis_queue.h"
#endif

 /* ################################################################### *
	* Definition of macros: per data structure
* ################################################################### */

#define DS_ADD(s,k,v)       push(s,k,v)
#define DS_REMOVE(s)        pop(s)
#define DS_SIZE(s)          stack_size(s)
#define DS_NEW(n,w,d,b,m,k)       create_stack(n,w,d,b,m,k)
#define DS_REGISTER(s,i)    register_stack(s,i)

#define DS_TYPE             mstack_t
#define DS_HANDLE           mstack_t*
#define DS_NODE             node_t

/* Type definitions */
typedef struct mstack_node
{
	struct mstack_node* next;
	skey_t key;
	sval_t val;

	row_t next_count;	// The count the descriptor had when this was pushed. Could have been a gap.

	uint8_t padding[CACHE_LINE_SIZE - sizeof(skey_t) - sizeof(sval_t) - sizeof(struct mstack_node*) - sizeof(row_t)];
} node_t;

typedef struct file_descriptor
{
	node_t* node;
	uint64_t count;
} descriptor_t;

typedef ALIGNED(CACHE_LINE_SIZE) struct array_index
{
	volatile descriptor_t descriptor;
	uint8_t padding[CACHE_LINE_SIZE - sizeof(descriptor_t)];
} index_t;

typedef ALIGNED(CACHE_LINE_SIZE) struct mstack_file
{
	index_t *set_array;
	lateral_stack_t* lateral;
	uint64_t random_hops;
	uint64_t relaxation_bound;
	volatile depth_t depth;
	volatile width_t width;
	width_t max_width;
	uint8_t k_mode;
	uint8_t padding[CACHE_LINE_SIZE - sizeof(index_t*) - sizeof(lateral_stack_t*) - sizeof(uint64_t)*2 - sizeof(depth_t) - 2*sizeof(width_t) - sizeof(uint8_t)];
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
extern __thread unsigned long my_slide_fail_count;

/* Interfaces */
int push(mstack_t *set, skey_t key, sval_t val);
sval_t pop(mstack_t *set);
node_t* create_node(skey_t key, sval_t val, node_t* next);
mstack_t* create_stack(size_t num_threads, width_t width, depth_t depth, width_t max_width, uint8_t k_mode, uint64_t relaxation_bound);
mstack_t* register_stack(mstack_t *set, int thread_id);
size_t stack_size(mstack_t *set);
int floor_log_2(unsigned int n);
depth_t update_depth(mstack_t *set, depth_t depth);
width_t update_width(mstack_t *set, width_t width);
