#include "partial-ms.h"

#ifdef RELAXATION_ANALYSIS
#include "relaxation_analysis_queue.c"
#elif RELAXATION_TIMER_ANALYSIS
#include "relaxation_analysis_timestamps.c"
#endif


node_t* create_ms_node(skey_t key, sval_t val, node_t* next)
{
	#if GC == 1
		node_t *node = ssmem_alloc(alloc, sizeof(node_t));
	#else
	  	node_t* node = ssalloc(sizeof(node_t));
	#endif
	node->key = key;
	node->val = val;
	node->next = next;

	return node;
}

void init_ms_queue(ms_queue_t *q) {
		// Does it work with ssmem to mix allocated mem with an alloc and without?
		node_t* node = (node_t*) ssalloc_aligned(CACHE_LINE_SIZE, sizeof(node_t));
		node->next = NULL;
		descriptor_t init_desc;
		init_desc.count = 0;
		init_desc.node = node;
		q->head = init_desc;
		q->tail = init_desc;
}

static int enq_cae(node_t** next_node_loc, node_t* new_node)
{
	node_t* expected = NULL;
#ifdef RELAXATION_TIMER_ANALYSIS
	// Use timers to track relaxation instead of locks
	if (CAE(next_node_loc, &expected, &new_node))
	{
		// Save this count in a local array of (timestamp, )
		add_relaxed_put(new_node->val, get_timestamp());
		return true;
	}
	return false;

#elif RELAXATION_ANALYSIS

	lock_relaxation_lists();

	if (CAE(next_node_loc, &expected, &new_node))
	{
		new_node->val = gen_relaxation_count();
		add_linear(new_node->val, 0);
		unlock_relaxation_lists();
		return true;
	}
	else {
		unlock_relaxation_lists();
		return false;
	}

#else
	return CAE(next_node_loc, &expected, &new_node);
#endif
}

static int deq_cae(volatile descriptor_t* des_loc, descriptor_t* read_des_loc, descriptor_t* new_des_loc)
{
#ifdef RELAXATION_TIMER_ANALYSIS
	// Use timers to track relaxation instead of locks
	if (CAE(des_loc, read_des_loc, new_des_loc))
	{
		// TODO: Should we take the timestamp at another point in time?
		add_relaxed_get(new_des_loc->node->val, get_timestamp());
		return true;
	}
	return false;

#elif RELAXATION_ANALYSIS

	lock_relaxation_lists();
	if (CAE(des_loc, read_des_loc, new_des_loc))
	{
		remove_linear(new_des_loc->node->val);
		unlock_relaxation_lists();
		return true;
	}
	else {
		unlock_relaxation_lists();
		return false;
	}

#else
	return CAE(des_loc, read_des_loc, new_des_loc);
#endif
}

int ms_enqueue(ms_queue_t *q, skey_t key, sval_t val)
{
    node_t* new_node = create_ms_node(key, val, NULL);
	descriptor_t tail;

    while(1)
	{
		tail = q->tail;
		if(tail.node->next == NULL)
		{
			if(enq_cae((node_t **) &tail.node->next, new_node))
			{
				break;
			}
		}
		else
		{
            descriptor_t new_tail;
            new_tail.count = tail.count + 1;
            new_tail.node = tail.node->next;
			CAE(&q->tail, &tail, &new_tail);
		}

		my_put_cas_fail_count+=1;
	}
    descriptor_t new_tail;
    new_tail.count = tail.count + 1;
    new_tail.node = new_node;
	CAE(&q->tail, &tail, &new_tail);
	return 1;
}

sval_t ms_dequeue(ms_queue_t *q)
{
	descriptor_t head, tail, new_tail, new_head;

	uint64_t index;
	sval_t val;
	while (1)
    {
		head = q->head;
		tail = q->tail;

		if (unlikely(head.node == tail.node))
		{
			if(head.node->next == NULL)
			{
				my_null_count+=1;
				return 0;
			}
			else
			{
				new_tail.count = tail.count + 1;
                new_tail.node = tail.node->next;
				CAE(&q->tail, &tail, &new_tail);
			}
		}
		else
		{
			new_head.count = head.count + 1;
            new_head.node = head.node->next;
			if(deq_cae((descriptor_t*) &q->head, &head, &new_head))
			{
				val = head.node->next->val;
				#if GC == 1
					ssmem_free(alloc, (void*) head.node);
				#endif
				return val;
			}
		}
    }
}

size_t ms_queue_size(ms_queue_t *q){
	return q->tail.count - q->head.count;
}

uint64_t ms_enq_count(ms_queue_t *q) {
	return q->tail.count;
}

uint64_t ms_deq_count(ms_queue_t *q) {
	return q->head.count;
}

