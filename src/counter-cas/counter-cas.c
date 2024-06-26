
#include "counter-cas.h"

RETRY_STATS_VARS;

#include "latency.h"

#if LATENCY_PARSING == 1
	__thread size_t lat_parsing_get = 0;
	__thread size_t lat_parsing_put = 0;
	__thread size_t lat_parsing_rem = 0;
#endif	/* LATENCY_PARSING == 1 */

extern __thread unsigned long* seeds;
__thread ssmem_allocator_t* alloc;

counter_t* create_counter()
{
	counter_t *set;

    ssalloc_init();

	if ((set = (counter_t*) ssalloc_aligned(CACHE_LINE_SIZE, sizeof(counter_t))) == NULL)
    {
		perror("malloc");
		exit(1);
    }
	set->count = 0;

	return set;
}


uint64_t increment(counter_t *set)
{
	while (1)
	{
		uint64_t count = set->count;
		uint64_t new = count + 1;
		if (CAE(&set->count, &count, &new))
		{
			return new;
		}
	}
}

uint64_t decrement(counter_t *set)
{
	while (1)
	{
		uint64_t count = set->count;
		if (count == 0)
		{
			return 0;
		}
		uint64_t new = count - 1;
		if (CAE(&set->count, &count, &new))
		{
#if VALIDATESIZE == 1
			return 1;
#else
			return new;
#endif
		}
	}
}

size_t counter_size(counter_t *set)
{
	return set->count;
}

counter_t* counter_register(counter_t *set, int thread_id)
{
    return set;
}