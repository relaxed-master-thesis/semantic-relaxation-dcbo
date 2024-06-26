#include "d-balanced-queue.h"

// Internal thread local count for double-collect
// Don't have in header as it would double-instantiate both here and in the test file
__thread uint64_t *double_collect_counts;
__thread ssmem_allocator_t* alloc;
__thread handle_t* thread_handles;


int enqueue(mqueue_t *set, skey_t key, sval_t val) {
    #ifdef LENGTH_HEURISTIC
    #define ENQ_HEURISTIC(q) PARTIAL_LENGTH(q)
    #else
    #define ENQ_HEURISTIC(q) PARTIAL_ENQ_COUNT(q)
    #endif

    uint32_t opt_index = random_index(set);
    uint64_t opt = ENQ_HEURISTIC(&set->queues[opt_index]);
    for(int i = 1; i < set->d; i++ )
    {
        uint32_t index = random_index(set);
        uint64_t index_val = ENQ_HEURISTIC(&set->queues[index]);
        if(index_val < opt)
        {
            opt_index = index;
            opt = index_val;
        }
    }

    return PARTIAL_ENQUEUE(&set->queues[opt_index], key, val, opt_index);
}

sval_t dequeue(mqueue_t *set) {
    #ifdef LENGTH_HEURISTIC
    #define DEQ_HEURISTIC(q) -PARTIAL_LENGTH(q)
    #else
    #define DEQ_HEURISTIC(q) PARTIAL_DEQ_COUNT(q)
    #endif

    uint32_t opt_index = random_index(set);
    int64_t opt = DEQ_HEURISTIC(&set->queues[opt_index]);
    for(int i = 1; i < set->d; i++ )
    {
        uint32_t index = random_index(set);
        int64_t index_val = DEQ_HEURISTIC(&set->queues[index]);
        if(index_val < opt)
        {
            opt_index = index;
            opt = index_val;
        }
    }

    sval_t v = PARTIAL_DEQUEUE(&(set->queues[opt_index]), opt_index);
    if(v != EMPTY) return v;
    return double_collect(set, opt_index + 1);
}

sval_t double_collect(mqueue_t *set, uint32_t start_index){
    uint32_t index;
    uint64_t throwaway;

    start:
    // Loop through all, collecting their tail versions and then try to dequeue if not empty
    for(uint32_t i = 0; i<set->width; i++){
        index = (start_index + i) % set->width; // TODO: Optimize away modulo

        double_collect_counts[index] = PARTIAL_TAIL_VERSION(&set->queues[index]);
        sval_t v = PARTIAL_DEQUEUE(&(set->queues[index]), index);
        if(v != EMPTY) return v;
    }

    // Return empty if all counts are the same and the queues are still empty, otherwise restart
    for(uint32_t i = 0; i<set->width; i++){
        index = (start_index + i) % set->width;
        if (double_collect_counts[index] != PARTIAL_TAIL_VERSION(&(set->queues[index])))
        {
            start_index = index;
            goto start;
        }
    }

    return EMPTY;
}

mqueue_t* create_queue(uint32_t n_partial, uint32_t d, int nbr_threads)
{
    //Allocate n_partial MS
    mqueue_t *set;

	// Create an allocator for the main thread to more easily allocate the first queue node
    ssalloc_init();
	#if GC == 1
    if (alloc == NULL)
    {
		alloc = (ssmem_allocator_t*) malloc(sizeof(ssmem_allocator_t));
		assert(alloc != NULL);
		ssmem_alloc_init_fs_size(alloc, SSMEM_DEFAULT_MEM_SIZE, SSMEM_GC_FREE_SET_SIZE, nbr_threads);
    }
	#endif

	if ((set = (mqueue_t*) ssalloc_aligned(CACHE_LINE_SIZE, sizeof(mqueue_t))) == NULL)
    {
		perror("malloc");
		exit(1);
    }
	set->queues = ssalloc_aligned(CACHE_LINE_SIZE, n_partial*sizeof(PARTIAL_T)); //ssalloc(width);
	set->width = n_partial;
    set->d = d;


	uint32_t i;
	for(i=0; i < set->width; i++)
	{
        INIT_PARTIAL(&(set->queues[i]), nbr_threads);
	}

	return set;
}

size_t queue_size(mqueue_t *set)
{
    uint64_t total = 0;
    for(int i=0; i<set->width; i++){
        total+=PARTIAL_LENGTH(&set->queues[i]);
    }
    return total;
}

uint32_t random_index(mqueue_t *set)
{
	return (my_random(&(seeds[0]), &(seeds[1]), &(seeds[2])) % (set->width));
}

// Set up thread local variables for the queue
mqueue_t* d_balanced_register(mqueue_t *set, int thread_id)
{
    ssalloc_init();
	#if GC == 1
    if (alloc == NULL)
    {
		alloc = (ssmem_allocator_t*) malloc(sizeof(ssmem_allocator_t));
		assert(alloc != NULL);
		ssmem_alloc_init_fs_size(alloc, SSMEM_DEFAULT_MEM_SIZE, SSMEM_GC_FREE_SET_SIZE, thread_id);
    }
	#endif

	double_collect_counts = malloc(set->width*sizeof(uint64_t));
    thread_handles = malloc(set->width*sizeof(handle_t));
    for (int i = 0; i < set->width; i++)
    {
        wfqueue_register(&set->queues[i], &thread_handles[i], thread_id);
    }
#ifdef RELAXATION_TIMER_ANALYSIS
	init_relaxation_analysis_local(thread_id);
#endif
    return set;
}