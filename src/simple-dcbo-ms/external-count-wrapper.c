#include "external-count-wrapper.h"

int wrapped_enqueue(wrapped_queue_t *queue, skey_t key, sval_t val)
{
    ms_enqueue(&queue->partial, key, val);
    FAI_U64(&queue->enq_count);
    return 1;
}

sval_t wrapped_dequeue(wrapped_queue_t *queue)
{
    sval_t ret = ms_dequeue(&queue->partial);
    if (ret != EMPTY)
    {
        // Count number of successful dequeues only
        FAI_U64(&queue->deq_count);
    }
    return ret;
}

void init_wrapped_queue(wrapped_queue_t *queue)
{
    init_ms_queue(&queue->partial);
    queue->deq_count = 0;
    queue->enq_count = 0;
}

// Uses external counters, so not linearizable
size_t wrapped_queue_size(wrapped_queue_t *queue)
{
    // return ms_queue_size(&queue->partial);
    uint64_t enq_count = queue->enq_count;
    uint64_t deq_count = queue->deq_count;
    if (deq_count > enq_count)
    {
        return 0;
    }
    return enq_count - deq_count;
}

int wrapped_tail_version(wrapped_queue_t *queue)
{
    // Just need to be uniquely updated every enqueue. So could for example be the address of the tail descriptor
    return ms_enq_count(&queue->partial);
}

uint64_t wrapped_enq_count(wrapped_queue_t *queue)
{
    return queue->enq_count;
}

uint64_t wrapped_deq_count(wrapped_queue_t *queue)
{
    return queue->deq_count;
}
