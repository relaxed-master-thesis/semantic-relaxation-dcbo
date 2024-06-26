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

 /* ################################################################### *
	* Definition of macros: per data structure
* ################################################################### */

#define DS_ADD(s,k,v)       increment(s)
#define DS_REMOVE(s)        decrement(s)
#define DS_SIZE(s)          counter_size(s)
#define DS_NEW()            create_counter()
#define DS_REGISTER(s,i)    s

#define DS_TYPE             counter_t
#define DS_HANDLE           counter_t*
// Not really working as intended
#define DS_NODE             uint64_t

/* Type definitions */
typedef ALIGNED(CACHE_LINE_SIZE) struct counter
{
	uint64_t count;
	uint8_t padding[CACHE_LINE_SIZE - sizeof(uint64_t)];
} counter_t;

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
uint64_t increment(counter_t *set);
uint64_t decrement(counter_t *set);
counter_t* create_counter();
size_t counter_size(counter_t *set);
int floor_log_2(unsigned int n);
