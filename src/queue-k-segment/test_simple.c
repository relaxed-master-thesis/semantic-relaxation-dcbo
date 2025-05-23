/*
 *   File: test_simple.c
 *   Author: Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>
 *   Description:
 *   test_simple.c is part of ASCYLIB
 *
 * Copyright (c) 2014 Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>,
 * 	     	      Tudor David <tudor.david@epfl.ch>
 *	      	      Distributed Programming Lab (LPD), EPFL
 *
 * ASCYLIB is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, version 2
 * of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <assert.h>
#include <getopt.h>
#include <limits.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sched.h>
#include <inttypes.h>
#include <sys/time.h>
#include <unistd.h>
#include <malloc.h>
#include "utils.h"

#include "rapl_read.h"
#ifdef __sparc__
	#include <sys/types.h>
	#include <sys/processor.h>
	#include <sys/procset.h>
#endif

#include "intset.h"

//ad
#if defined(RELAXATION_ANALYSIS)
	#include "relaxation_analysis_queue.h"
#endif
#if !defined(VALIDATESIZE)
	#define VALIDATESIZE 1
#endif

/* ################################################################### *
 * Definition of macros: per data structure
 * ################################################################### */

#define DS_CONTAINS(s,k,t)  queue_contains(s, k)
#define DS_ADD(s,k,t)       queue_add(s, k, t)
#define DS_REMOVE(s)        queue_remove(s)
#define DS_SIZE(s)          queue_size(s)
#define DS_REGISTER(s,i)    s
#define DS_NEW()            queue_new()

#define DS_HANDLE           queue_t*
#define DS_TYPE             queue_t
#define DS_NODE             queue_node_t

/* ################################################################### *
 * GLOBALS
 * ################################################################### */

RETRY_STATS_VARS_GLOBAL;

size_t initial = DEFAULT_INITIAL;
size_t range = DEFAULT_RANGE;
size_t update = 100;
size_t load_factor;
size_t num_threads = DEFAULT_NB_THREADS;
size_t duration = DEFAULT_DURATION;
size_t side_work = 0;

size_t print_vals_num = 100;
size_t pf_vals_num = 1023;
size_t put, put_explicit = false;
double update_rate, put_rate, get_rate;

size_t size_after = 0;
int seed = 0;
__thread unsigned long * seeds;
uint32_t rand_max;
#define rand_min 1

static volatile int stop;

//ad
double *node_tag;
__thread int thread_id;
__thread unsigned long push_slot;
__thread unsigned long pop_slot;
__thread unsigned long walk_count;
__thread unsigned long my_push_cas_fail_count;
__thread unsigned long my_pop_cas_fail_count;
__thread unsigned long my_null_count;
__thread unsigned long my_hop_count;
__thread unsigned long my_slide_count;
int segment_size=1;
int relaxation_bound = 1;
volatile segment_t* head;
volatile segment_t* tail;
__thread segment_t* current_segment;
__thread queue_node_t* segment_status;
__thread queue_node_t* current_node;
__thread uint64_t deleted_node;
__thread ssmem_allocator_t* alloc_segment;
__thread ssmem_allocator_t* alloc;

////////////////////////////////////ad end

TEST_VARS_GLOBAL;

volatile ticks *putting_succ;
volatile ticks *putting_fail;
volatile ticks *removing_succ;
volatile ticks *removing_fail;
volatile ticks *putting_count;
volatile ticks *putting_count_succ;
volatile unsigned long *push_cas_fail_count;
volatile unsigned long *pop_cas_fail_count;
volatile unsigned long *null_count;
volatile unsigned long *hop_count;
volatile unsigned long *slide_count;
volatile ticks *removing_count;
volatile ticks *removing_count_succ;
volatile ticks *total;


/* ################################################################### *
 * LOCALS
 * ################################################################### */

#ifdef DEBUG
	extern __thread uint32_t put_num_restarts;
	extern __thread uint32_t put_num_failed_expand;
	extern __thread uint32_t put_num_failed_on_new;
#endif

barrier_t barrier, barrier_global;

typedef struct thread_data
{
	uint32_t id;
	DS_TYPE* set;
} thread_data_t;

void* test(void* thread)
{
	thread_data_t* td = (thread_data_t*) thread;
	uint32_t ID = td->id;
	thread_id=ID;
	set_cpu(ID);

	//push_slot=thread_id*(segment_size/num_threads);
	//pop_slot=thread_id*(segment_size/num_threads);

	DS_TYPE* set = td->set;

	THREAD_INIT(ID);
#ifdef RELAXATION_TIMER_ANALYSIS
	if (thread_id == 0) init_relaxation_analysis_shared(num_threads);
#endif
	PF_INIT(3, SSPFD_NUM_ENTRIES, ID);

	#if defined(COMPUTE_LATENCY)
		volatile ticks my_putting_succ = 0;
		volatile ticks my_putting_fail = 0;
		volatile ticks my_removing_succ = 0;
		volatile ticks my_removing_fail = 0;
	#endif
	uint64_t my_putting_count = 0;
	uint64_t my_removing_count = 0;

	uint64_t my_putting_count_succ = 0;
	uint64_t my_removing_count_succ = 0;

	#if defined(COMPUTE_LATENCY) && PFD_TYPE == 0
		volatile ticks start_acq, end_acq;
		volatile ticks correction = getticks_correction_calc();
	#endif

	seeds = seed_rand();
	#if GC == 1
		alloc = (ssmem_allocator_t*) malloc(sizeof(ssmem_allocator_t));
		assert(alloc != NULL);
		ssmem_alloc_init_fs_size(alloc, SSMEM_DEFAULT_MEM_SIZE, SSMEM_GC_FREE_SET_SIZE, ID);

		alloc_segment = (ssmem_allocator_t*) malloc(sizeof(ssmem_allocator_t));
		assert(alloc_segment != NULL);
		ssmem_alloc_init_fs_size(alloc_segment, SSMEM_DEFAULT_MEM_SIZE, SSMEM_GC_FREE_SET_SIZE, ID);
	#endif


	RR_INIT(thread_id);//used by rapl_read
	barrier_cross(&barrier);

	DS_HANDLE handle = DS_REGISTER(set, thread_id);

#ifdef RELAXATION_TIMER_ANALYSIS
	init_relaxation_analysis_local(thread_id);
#endif

	uint64_t key;
	int c = 0;
	uint32_t scale_rem = (uint32_t) (update_rate * UINT_MAX);
	uint32_t scale_put = (uint32_t) (put_rate * UINT_MAX);

	int i;
	uint32_t num_elems_thread = (uint32_t) (initial / num_threads);
	int32_t missing = (uint32_t) initial - (num_elems_thread * num_threads);
	if (ID < missing)
    {
		num_elems_thread++;
    }

	//ad this forces thread id=0 to be the only one to initialise the data set
	#if INITIALIZE_FROM_ONE == 1
		num_elems_thread = (ID == 0) * initial;
	#endif

	for(i = 0; i < num_elems_thread; i++)
    {
		// key = (my_random(&(seeds[0]), &(seeds[1]), &(seeds[2])) % (rand_max + 1)) + rand_min;
		key = i << 8 | thread_id;


		if(DS_ADD(handle, key, key) == false)
		{
			i--;
		}
    }

	MEM_BARRIER;
	barrier_cross(&barrier);//all threads will wait here for initialisation to finish
	if (!ID)
    {
		printf("BEFORE size is, %zu\n", (size_t) DS_SIZE(set));
    }
	RETRY_STATS_ZERO(); //ad for latency.h
	barrier_cross(&barrier_global);
	RR_START_SIMPLE(); //ad for rapl_read
	while (stop == 0)
    {
		TEST_LOOP_ONLY_UPDATES(); //run only update tests
    }
	barrier_cross(&barrier);
	RR_STOP_SIMPLE(); //ad for rapl_read
	if (!ID)
    {
		size_after = DS_SIZE(set);
		printf("AFTER size is, %zu \n", size_after);
    }

	barrier_cross(&barrier);

	#if defined(COMPUTE_LATENCY)
		putting_succ[ID] += my_putting_succ;
		putting_fail[ID] += my_putting_fail;
		removing_succ[ID] += my_removing_succ;
		removing_fail[ID] += my_removing_fail;
	#endif
	putting_count[ID] += my_putting_count;
	removing_count[ID]+= my_removing_count;

	putting_count_succ[ID] += my_putting_count_succ;
	removing_count_succ[ID]+= my_removing_count_succ;

	//ad added
	push_cas_fail_count[thread_id]=my_push_cas_fail_count;
	pop_cas_fail_count[thread_id]=my_pop_cas_fail_count;
	null_count[thread_id]=my_null_count;
	hop_count[thread_id]=my_hop_count;
	slide_count[thread_id]=my_slide_count;

	EXEC_IN_DEC_ID_ORDER(ID, num_threads)
    {
		print_latency_stats(ID, SSPFD_NUM_ENTRIES, print_vals_num);
		RETRY_STATS_SHARE();
    }
	EXEC_IN_DEC_ID_ORDER_END(&barrier);

	SSPFDTERM();
	#if GC == 1
		ssmem_term();
		free(alloc);
	#endif
	THREAD_END();
	pthread_exit(NULL);
}

int main(int argc, char **argv)
{
  //set_cpu(0);
  ssalloc_init();
  seeds = seed_rand();

  struct option long_options[] = {
    // These options don't set a flag
    {"help",                      no_argument,       NULL, 'h'},
    {"duration",                  required_argument, NULL, 'd'},
    {"initial-size",              required_argument, NULL, 'i'},
    {"num-threads",               required_argument, NULL, 'n'},
    {"range",                     required_argument, NULL, 'r'},
    {"update-rate",               required_argument, NULL, 'u'},
    {"num-buckets",               required_argument, NULL, 'b'},
    {"print-vals",                required_argument, NULL, 'v'},
    {"vals-pf",                   required_argument, NULL, 'f'},
    {NULL, 0, NULL, 0}
  };

  int i, c;
  while(1)
    {
		i = 0;
		c = getopt_long(argc, argv, "hAf:d:i:n:r:s:u:m:a:l:p:b:v:f:k:m:w:", long_options, &i);
		if(c == -1)
			break;
		if(c == 0 && long_options[i].flag == 0)
			c = long_options[i].val;
		switch(c)
		{
			case 0:
				/* Flag is automatically set */
				break;
			case 'h':
				printf("ASCYLIB -- stress test "
					 "\n"
					 "\n"
					 "Usage:\n"
					 "  %s [options...]\n"
					 "\n"
					 "Options:\n"
					 "  -h, --help\n"
					 "        Print this message\n"
					 "  -d, --duration <int>\n"
					 "        Test duration in milliseconds\n"
					 "  -i, --initial-size <int>\n"
					 "        Number of elements to insert before test\n"
					 "  -n, --num-threads <int>\n"
					 "        Number of threads\n"
					 "  -r, --range <int>\n"
					 "        Range of integer values inserted in set\n"
					 "  -u, --update-rate <int>\n"
					 "        Percentage of update transactions\n"
					 "  -p, --put-rate <int>\n"
					 "        Percentage of put update transactions (should be less than percentage of updates)\n"
					 "  -b, --num-buckets <int>\n"
					 "        Number of initial buckets (stronger than -l)\n"
					 "  -v, --print-vals <int>\n"
					 "        When using detailed profiling, how many values to print.\n"
					 "  -f, --val-pf <int>\n"
					 "        When using detailed profiling, how many values to keep track of.\n"
					 "  -k, --Relaxation Bound <int>\n"
					 "        Relaxation bound.\n"
					 , argv[0]);
				exit(0);
			case 'd':
				duration = atoi(optarg);
				break;
			case 'i':
				initial = atoi(optarg);
				break;
			case 'n':
				num_threads = atoi(optarg);
				break;
			case 'r':
				range = atol(optarg);
				break;
			case 'u':
				update = atoi(optarg);
				break;
			case 'p':
				put_explicit = 1;
				put = atoi(optarg);
				break;
			case 'l':
				load_factor = atoi(optarg);
				break;
			case 'v':
				print_vals_num = atoi(optarg);
				break;
			case 'f':
				pf_vals_num = pow2roundup(atoi(optarg)) - 1;
				break;
			case 'k':
				relaxation_bound = atoi(optarg);
				break;
			case 'm':
			case 'w':
				// Does not do anything, but added to play nicer in tests with other relaxed data structures
				break;
			case '?':
			default:
				printf("Use -h or --help for help\n");
				exit(1);
		}
    }


	if (!is_power_of_two(initial))
    {
		size_t initial_pow2 = pow2roundup(initial);
		printf("** rounding up initial (to make it power of 2): old: %zu / new: %zu\n", initial, initial_pow2);
		initial = initial_pow2;
    }

	if (range < initial)
    {
		range = 2 * initial;
    }

	printf("Initial, %zu \n", initial);
	printf("Range, %zu \n", range);
	printf("Algorithm, OPTIK \n");

	double kb = initial * sizeof(DS_NODE) / 1024.0;
	double mb = kb / 1024.0;
	//printf("Sizeof initial, %.2f KB is %.2f MB\n", kb, mb);

	if (!is_power_of_two(range))
    {
		size_t range_pow2 = pow2roundup(range);
		printf("** rounding up range (to make it power of 2): old: %zu / new: %zu\n", range, range_pow2);
		range = range_pow2;
    }

	if (put > update)
    {
		put = update;
    }

	update_rate = update / 100.0;

	if (put_explicit)
    {
		put_rate = put / 100.0;
    }
	else
    {
		put_rate = update_rate / 2;
    }

	get_rate = 1 - update_rate;

  /* printf("num_threads = %u\n", num_threads); */
  /* printf("cap: = %u\n", num_buckets); */
  /* printf("num elem = %u\n", num_elements); */
  /* printf("filing rate= %f\n", filling_rate); */
  /* printf("update = %f (putting = %f)\n", update_rate, put_rate); */


	rand_max = range - 1;

	struct timeval start, end;
	struct timespec timeout;
	timeout.tv_sec = duration / 1000;
	timeout.tv_nsec = (duration % 1000) * 1000000;
	stop = 0;

	DS_TYPE* set = DS_NEW();
	assert(set != NULL);

	/* Initializes the local data */
	putting_succ = (ticks *) calloc(num_threads , sizeof(ticks));
	putting_fail = (ticks *) calloc(num_threads , sizeof(ticks));
	removing_succ = (ticks *) calloc(num_threads , sizeof(ticks));
	removing_fail = (ticks *) calloc(num_threads , sizeof(ticks));
	putting_count = (ticks *) calloc(num_threads , sizeof(ticks));
	putting_count_succ = (ticks *) calloc(num_threads , sizeof(ticks));
	push_cas_fail_count = (unsigned long *) calloc(num_threads , sizeof(unsigned long));
	pop_cas_fail_count = (unsigned long *) calloc(num_threads , sizeof(unsigned long));
	null_count = (unsigned long *) calloc(num_threads , sizeof(unsigned long));
	slide_count = (unsigned long *) calloc(num_threads , sizeof(unsigned long));
	hop_count = (unsigned long *) calloc(num_threads , sizeof(unsigned long));
	removing_count = (ticks *) calloc(num_threads , sizeof(ticks));
	removing_count_succ = (ticks *) calloc(num_threads , sizeof(ticks));

	//ad initialize start segment
	/* ***************************
	*******************************/
	#if defined(RELAXATION_ANALYSIS)
		init_relaxation_analysis();
	#endif
	segment_size = relaxation_bound + 1;
	index_t* indices_memory = (index_t *) calloc(segment_size, sizeof(index_t));
	segment_t* new_segment = (segment_t *) calloc(1, sizeof(segment_t));
	new_segment->next = NULL;
	new_segment->indices = indices_memory;

	head = new_segment;
	tail = new_segment;


	pthread_t threads[num_threads];
	pthread_attr_t attr;
	int rc;
	void *status;

	//ad initialize barriers
	barrier_init(&barrier_global, num_threads + 1);
	barrier_init(&barrier, num_threads);

	/* Initialize and set thread detached attribute */
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	thread_data_t* tds = (thread_data_t*) malloc(num_threads * sizeof(thread_data_t));

	long t;
	for(t = 0; t < num_threads; t++)
    {
		tds[t].id = t;
		tds[t].set = set;
		rc = pthread_create(&threads[t], &attr, test, tds + t); //ad create thread and call test function
		if (rc)
		{
			printf("ERROR; return code from pthread_create() is %d\n", rc);
			exit(-1);
		}
    }

	/* Free attribute and wait for the other threads */
	pthread_attr_destroy(&attr);
	/*main thread will wait on the &barrier_global until all threads within test have reached
	and set the timer before they cross to start the test loop*/
	barrier_cross(&barrier_global);
	gettimeofday(&start, NULL);
	nanosleep(&timeout, NULL);

	stop = 1;
	gettimeofday(&end, NULL);
	duration = (end.tv_sec * 1000 + end.tv_usec / 1000) - (start.tv_sec * 1000 + start.tv_usec / 1000);

	for(t = 0; t < num_threads; t++)
    {
		rc = pthread_join(threads[t], &status);
		if (rc)
		{
			printf("ERROR; return code from pthread_join() is %d\n", rc);
			exit(-1);
		}
    }

	free(tds);

	volatile ticks putting_suc_total = 0;
	volatile ticks putting_fal_total = 0;
	volatile ticks removing_suc_total = 0;
	volatile ticks removing_fal_total = 0;
	volatile uint64_t putting_count_total = 0;
	volatile uint64_t putting_count_total_succ = 0;
	volatile unsigned long push_cas_fail_count_total = 0;
	volatile unsigned long pop_cas_fail_count_total = 0;
	volatile unsigned long null_count_total = 0;
	volatile unsigned long slide_count_total = 0;
	volatile unsigned long hop_count_total = 0;
	volatile uint64_t removing_count_total = 0;
	volatile uint64_t removing_count_total_succ = 0;

	for(t=0; t < num_threads; t++)
    {
		PRINT_OPS_PER_THREAD();
		putting_suc_total += putting_succ[t];
		putting_fal_total += putting_fail[t];
		removing_suc_total += removing_succ[t];
		removing_fal_total += removing_fail[t];
		putting_count_total += putting_count[t];
		putting_count_total_succ += putting_count_succ[t];
		push_cas_fail_count_total += push_cas_fail_count[t];
		pop_cas_fail_count_total += pop_cas_fail_count[t];
		null_count_total += null_count[t];
		hop_count_total += hop_count[t];
		slide_count_total += slide_count[t];
		removing_count_total += removing_count[t];
		removing_count_total_succ += removing_count_succ[t];
    }

	#if defined(COMPUTE_LATENCY)
		printf("#thread srch_suc srch_fal insr_suc insr_fal remv_suc remv_fal   ## latency (in cycles) \n"); fflush(stdout);
		long unsigned put_suc = putting_count_total_succ ? putting_suc_total / putting_count_total_succ : 0;
		long unsigned put_fal = (putting_count_total - putting_count_total_succ) ? putting_fal_total / (putting_count_total - putting_count_total_succ) : 0;
		long unsigned rem_suc = removing_count_total_succ ? removing_suc_total / removing_count_total_succ : 0;
		long unsigned rem_fal = (removing_count_total - removing_count_total_succ) ? removing_fal_total / (removing_count_total - removing_count_total_succ) : 0;
		printf("%-7zu %-8lu %-8lu %-8lu %-8lu %-8lu %-8lu\n", num_threads, get_suc, get_fal, put_suc, put_fal, rem_suc, rem_fal);
	#endif

	#define LLU long long unsigned int

	int UNUSED pr = (int) (putting_count_total_succ - removing_count_total_succ);
	#if defined(VALIDATESIZE)
	if (size_after != (initial + pr))
    {
		printf("\n******** ERROR WRONG size. %zu + %d != %zu (difference %d)**********\n\n", initial, pr, size_after, (int)((initial + pr)-size_after));
		//assert(size_after == (initial + pr));
    }
	#endif
	uint64_t total = putting_count_total + removing_count_total;
	double putting_perc = 100.0 * (1 - ((double)(total - putting_count_total) / total));
	double putting_perc_succ = (1 - (double) (putting_count_total - putting_count_total_succ) / putting_count_total) * 100;
	double removing_perc = 100.0 * (1 - ((double)(total - removing_count_total) / total));
	double removing_perc_succ = (1 - (double) (removing_count_total - removing_count_total_succ) / removing_count_total) * 100;

	printf("putting_count_total , %-10llu \n", (LLU) putting_count_total);
	printf("putting_count_total_succ , %-10llu \n", (LLU) putting_count_total_succ);
	printf("putting_perc_succ , %10.1f \n", putting_perc_succ);
	printf("putting_perc , %10.1f \n", putting_perc);
	printf("putting_effective , %10.1f \n", (putting_perc * putting_perc_succ) / 100);

	printf("removing_count_total , %-10llu \n", (LLU) removing_count_total);
	printf("removing_count_total_succ , %-10llu \n", (LLU) removing_count_total_succ);
	printf("removing_perc_succ , %10.1f \n", removing_perc_succ);
	printf("removing_perc , %10.1f \n", removing_perc);
	printf("removing_effective , %10.1f \n", (removing_perc * removing_perc_succ) / 100);


	double throughput = (putting_count_total + removing_count_total_succ) * 1000.0 / duration;

	printf("num_threads , %zu \n", num_threads);
	printf("Mops , %.3f\n", throughput / 1e6);
	printf("Ops , %.2f\n", throughput);

	//RR_PRINT_UNPROTECTED(RAPL_PRINT_POW);
	RR_PRINT_CORRECTED();
	RETRY_STATS_PRINT(total, putting_count_total, removing_count_total, putting_count_total_succ + removing_count_total_succ);
	LATENCY_DISTRIBUTION_PRINT();

	#ifdef RELAXATION_TIMER_ANALYSIS
		print_relaxation_measurements(num_threads);
	#elif RELAXATION_ANALYSIS
		print_relaxation_measurements();
	#else
		printf("Push_CAS_fails , %zu\n", push_cas_fail_count_total);
		printf("Pop_CAS_fails , %zu\n", pop_cas_fail_count_total);
		printf("Null_Count , %zu\n", null_count_total);
		printf("Slide_Count , %zu\n", slide_count_total);
		printf("Hop_Count , %zu\n", hop_count_total);
	#endif

	pthread_exit(NULL);

	return 0;
}
