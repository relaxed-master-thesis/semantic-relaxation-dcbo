
#include "2Dc-counter.h"
#include "2Dc-window.c"

#ifdef RELAXATION_ANALYSIS
#include "relaxation_analysis_counter.h"
#elif RELAXATION_TIMER_ANALYSIS
#error "Timer analysis not supported for counter"
#endif

RETRY_STATS_VARS;

#include "latency.h"

#if LATENCY_PARSING == 1
	__thread size_t lat_parsing_get = 0;
	__thread size_t lat_parsing_put = 0;
	__thread size_t lat_parsing_rem = 0;
#endif	/* LATENCY_PARSING == 1 */

extern __thread unsigned long* seeds;
__thread ssmem_allocator_t* alloc;

counter_t* create_counter(size_t num_threads, uint64_t width, uint64_t depth, uint8_t k_mode, uint64_t relaxation_bound)
{
	counter_t *set;

    ssalloc_init();
	#if GC == 1
    if (alloc == NULL)
    {
		alloc = (ssmem_allocator_t*) malloc(sizeof(ssmem_allocator_t));
		assert(alloc != NULL);
		ssmem_alloc_init_fs_size(alloc, SSMEM_DEFAULT_MEM_SIZE, SSMEM_GC_FREE_SET_SIZE, thread_id);
    }
	#endif


	/****
		calculate width and depth using the relaxation bound (K = (2depth)(width−1))
	****/
	if(k_mode == 3)
	{
		//maximum width is fixed as a multiple of number of threads
		width = num_threads * width;
		if(width < 2 )
		{
			width  = 1;
			depth  = relaxation_bound;
			relaxation_bound = 0;
		}
		else
		{
			depth = relaxation_bound / (2*(width - 1));
			if(depth<1)
			{
				depth = 1;
				width = (relaxation_bound / (2*depth)) + 1;
			}
		}
	}
	else if(k_mode == 2)
	{
		//maximum depth is fixed
		width = (relaxation_bound / (2*depth)) + 1;
		if(width<1)
		{
			width = 1;
			depth  = relaxation_bound;
			relaxation_bound = 0;
		}
	}
	else if(k_mode == 1)
	{
		//width parameter is fixed
		if(width < 2 )
		{
			width  = 1;
			depth  = relaxation_bound;
			relaxation_bound = 0;
		}
		else
		{
			depth = relaxation_bound / (2*(width - 1));
			if(depth<1)
			{
				depth = 1;
				width = (relaxation_bound / (2*depth)) + 1;
			}
		}
	}
	else if(k_mode == 0)
	{
		relaxation_bound = 2 * depth * (width -1);
	}
	/*************************************************************/

	/*initialise window*/
	global_Window.content.max = depth;
	global_Window.content.version = 0;
	/*******************/

#ifdef RELAXATION_ANALYSIS
	init_relaxation_analysis();
#endif

	if ((set = (counter_t*) ssalloc_aligned(CACHE_LINE_SIZE, sizeof(counter_t))) == NULL)
    {
		perror("malloc");
		exit(1);
    }
	set->set_array = (index_t*)calloc(width, sizeof(index_t)); //ssalloc(width);
	set->width = width;
	set->depth = depth;
	set->shift = depth/2 > 1 ? depth/2 : 1;
	set->random_hops = 2;
	set->k_mode = k_mode;
	set->relaxation_bound = relaxation_bound;

	int i;
	for(i=0; i < set->width; i++)
	{
		set->set_array[i].descriptor.count = 0;
	}

	return set;
}

static inline char counter_cae(volatile descriptor_t* desc_loc, descriptor_t* desc_read, descriptor_t *desc_new, DS_TYPE* set)
{
#ifdef RELAXATION_ANALYSIS
	lock_relaxation_lists();
	if (CAE(desc_loc, desc_read, desc_new))
	{
		uint32_t count = desc_new->count * set->width;
		if (desc_new->count > desc_read->count)
		{
			inc_relaxed_count();
		} 
		else
		{
			dec_relaxed_count();
		}
		
		add_relaxed_count(count);
		unlock_relaxation_lists();
		return 1;
	}
	else 
	{
		unlock_relaxation_lists();
		return 0;
	}
#else
	return CAE(desc_loc, desc_read, desc_new);
#endif
}

uint64_t increment(counter_t *set)
{
	uint8_t contention = 0;
	descriptor_t descriptor, new_descriptor;
	while(1)
	{
		descriptor = put_window(set,contention);
		new_descriptor.count = descriptor.count + 1;
		if(counter_cae(&set->set_array[thread_index].descriptor,&descriptor,&new_descriptor, set))
		{
			#if VALIDATESIZE==1
				return 1;
			#else
				return (new_descriptor.count * set->width);
			#endif
		}
		else
		{
			contention = 1;
		}

		my_put_cas_fail_count+=1;
	}
}

uint64_t decrement(counter_t *set)
{
	uint8_t contention = 0;
	descriptor_t descriptor, new_descriptor;
	while (1)
    {
		descriptor = get_window(set, contention);
		if(descriptor.count > 0)
		{
			new_descriptor.count = descriptor.count - 1;
			if(counter_cae(&set->set_array[thread_index].descriptor,&descriptor,&new_descriptor, set))
			{
				#if VALIDATESIZE==1
					return 1;
				#else
					return (new_descriptor.count * set->width);
				#endif
			}
			else
			{
				contention = 1;
			}

			my_get_cas_fail_count+=1;
		}
		else
		{
			my_null_count+=1;
			return 0;
		}
    }
}

size_t counter_size(counter_t *set)
{
	size_t size = 0;
	uint64_t i;
	descriptor_t descriptor;
	for(i=0; i < set->width; i++)
	{
		descriptor = set->set_array[i].descriptor;
		size += (size_t)descriptor.count;
	}
	return size;
}

counter_t* counter_register(counter_t *set, int thread_id)
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

    return set;
}