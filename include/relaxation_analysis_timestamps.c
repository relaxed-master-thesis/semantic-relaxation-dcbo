#include "relaxation_analysis_timestamps.h"

// Thread local arrays for storing records
__thread relax_stamp_t* thread_put_stamps;
__thread size_t* thread_put_stamps_ind;
__thread relax_stamp_t* thread_get_stamps;
__thread size_t* thread_get_stamps_ind;

// Shared array of all threads records
relax_stamp_t** shared_put_stamps;
relax_stamp_t** shared_get_stamps;
size_t** shared_put_stamps_ind; // Array of pointers, to make it more thread local without dropping too early
size_t** shared_get_stamps_ind; // Array of pointers, to make it more thread local without dropping too early


// Get a timestamp from the realtime clock, shared accross processors
uint64_t get_timestamp() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);  // Get the current time
    return (uint64_t)ts.tv_sec * 1e9 + ts.tv_nsec;  // Convert seconds and nanoseconds to a single 64-bit number
}

// Add a put operation of a value with its timestamp
void add_relaxed_put(sval_t val, uint64_t timestamp)
{
    relax_stamp_t stamp;
    stamp.timestamp = timestamp;
    stamp.value = val;
    thread_put_stamps[*thread_put_stamps_ind] = stamp;
    *thread_put_stamps_ind += 1;
    if (*thread_put_stamps_ind > MAX_RELAX_COUNTS) {
        perror("Out of bounds on relaxation stamps\n");
        exit(1);
    }}

// Add a get operation of a value with its timestamp
void add_relaxed_get(sval_t val, uint64_t timestamp)
{
    relax_stamp_t stamp;
    stamp.timestamp = timestamp;
    stamp.value = val;
    thread_get_stamps[*thread_get_stamps_ind] = stamp;
    *thread_get_stamps_ind += 1;
    if (*thread_get_stamps_ind > MAX_RELAX_COUNTS) {
        perror("Out of bounds on relaxation stamps\n");
        exit(1);
    }
}


// Init the relaxation analysis, global variables before the thread local one
void init_relaxation_analysis_shared(int nbr_threads)
{
    shared_put_stamps = (relax_stamp_t**) calloc(nbr_threads, sizeof(relax_stamp_t**));
    shared_get_stamps = (relax_stamp_t**) calloc(nbr_threads, sizeof(relax_stamp_t**));
    shared_put_stamps_ind = (size_t**) calloc(nbr_threads, sizeof(size_t**));
    shared_get_stamps_ind = (size_t**) calloc(nbr_threads, sizeof(size_t**));
}

// Init the relaxation analysis, thread local variables
void init_relaxation_analysis_local(int thread_id)
{
    thread_put_stamps_ind = (size_t*) calloc(1, sizeof(size_t));
    thread_get_stamps_ind = (size_t*) calloc(1, sizeof(size_t));
    thread_put_stamps = (relax_stamp_t*) calloc(MAX_RELAX_COUNTS, sizeof(relax_stamp_t));
    thread_get_stamps = (relax_stamp_t*) calloc(MAX_RELAX_COUNTS, sizeof(relax_stamp_t));

    if (thread_put_stamps == NULL || thread_get_stamps == NULL)
    {
        perror("Could not allocated thread local relaxation timestamp slots");
        exit(1);
    }
    shared_put_stamps[thread_id] = thread_put_stamps;
    shared_get_stamps[thread_id] = thread_get_stamps;
    shared_put_stamps_ind[thread_id] = thread_put_stamps_ind;
    shared_get_stamps_ind[thread_id] = thread_get_stamps_ind;

}

// de-init all memory for all threads
void destoy_relaxation_analysis_all(int nbr_threads)
{
    for (int thread = 0; thread < nbr_threads; thread += 1)
    {
        free(shared_get_stamps[thread]);
        free(shared_put_stamps[thread]);
        free(shared_get_stamps_ind[thread]);
        free(shared_put_stamps_ind[thread]);
    }
    free(shared_put_stamps);
    free(shared_get_stamps);
    free(shared_put_stamps_ind);
    free(shared_get_stamps_ind);
}

int compare_timestamps(const void *a, const void *b) {
    const relax_stamp_t *stamp1 = (const relax_stamp_t *)a;
    const relax_stamp_t *stamp2 = (const relax_stamp_t *)b;
    if (stamp1->timestamp < stamp2->timestamp) return -1;
    if (stamp1->timestamp > stamp2->timestamp) return 1;
    return 0;
}

relax_stamp_t* combine_sort_relaxed_stamps(int nbr_threads, relax_stamp_t** stamps, size_t** counts, size_t* tot_counts_out)
{
    *tot_counts_out = 0;
    for (int thread = 0; thread < nbr_threads; thread += 1)
    {
        *tot_counts_out += *counts[thread];
    }

    relax_stamp_t* combined_stamps = (relax_stamp_t*) calloc(*tot_counts_out, sizeof(relax_stamp_t));
    if (combined_stamps == NULL) {
        fprintf(stderr, "Memory allocation failed for combining relaxation errors\n");
        exit(1);
    }

    // Combine all lists into one
    size_t offset = 0;
    for (int thread = 0; thread < nbr_threads; thread++) {
        memcpy(combined_stamps + offset, stamps[thread], *counts[thread] * sizeof(relax_stamp_t));
        offset += *counts[thread];
    }

    // Sort the combined list using the comparator
    // TODO: In case this is too slow, we can get complecity to O(stamps*log(threads)) instead of O(stamps*log(stamps)) with some extra code
    qsort(combined_stamps, *tot_counts_out, sizeof(relax_stamp_t), compare_timestamps);

    return combined_stamps;
}

struct item_list {
    struct item_list *next;
    sval_t value;
};
void save_timestamps(relax_stamp_t* combined_put_stamps, size_t tot_put, relax_stamp_t* combined_get_stamps, size_t tot_get)
{
    //create dir if not exists
    system("mkdir -p results/timestamps");
    FILE* fptr;
    fptr = fopen("results/timestamps/combined_put_stamps.txt", "wb");
    //make all timestamps uniqe
    int keep_going = 1;
    uint64_t time = 0;
    uint64_t put_idx = 0;
    uint64_t get_idx = 0;
    uint64_t next_put_time = combined_put_stamps[0].timestamp;
    uint64_t next_get_time = combined_get_stamps[0].timestamp;

    printf("Removing duplicate timestamps...\n");
    while (keep_going) {
        while(put_idx < tot_put && next_put_time <= next_get_time) {
            combined_put_stamps[put_idx++].timestamp = time++;
            if(put_idx >= tot_put) {
                keep_going = 0;
                break;
            }
            next_put_time = combined_put_stamps[put_idx].timestamp;
        }
        while(next_get_time < next_put_time || put_idx >= tot_put) {
            combined_get_stamps[get_idx++].timestamp = time++;
            if (get_idx >= tot_get) {
                keep_going = 0;
                break;
            }
            next_get_time = combined_get_stamps[get_idx].timestamp;
        }
    }
    printf("Saving timestamps...\n");
    for(size_t idx = 0; idx < tot_put; idx++)
    {
        relax_stamp_t curr = combined_put_stamps[idx];
        if (unlikely(idx == tot_put - 1)) {
            fprintf(fptr,"%ld %ld", curr.timestamp, curr.value); 
        } else{
            fprintf(fptr,"%ld %ld\n", curr.timestamp, curr.value); 
        }
    }
    fclose(fptr); 
    fptr = fopen("results/timestamps/combined_get_stamps.txt", "wb");
    for(size_t idx = 0; idx < tot_get; idx++)
    {
        relax_stamp_t curr = combined_get_stamps[idx];
        if (unlikely(idx == tot_get - 1)) {
            fprintf(fptr,"%ld %ld", curr.timestamp, curr.value); 
        } else {
            fprintf(fptr,"%ld %ld\n", curr.timestamp, curr.value); 
        }
    }
    fclose(fptr); 
    printf("Timestamps saved.\n");
}
// Print the stats from the relaxation measurement. Also destroys all memory
void print_relaxation_measurements(int nbr_threads)
{
    // Sort all enqueue and dequeue operations in ascending order by time
    size_t tot_put, tot_get;

    relax_stamp_t* combined_put_stamps = combine_sort_relaxed_stamps(nbr_threads, shared_put_stamps, shared_put_stamps_ind, &tot_put);
    relax_stamp_t* combined_get_stamps = combine_sort_relaxed_stamps(nbr_threads, shared_get_stamps, shared_get_stamps_ind, &tot_get);


#ifdef SAVE_TIMESTAMPS
    save_timestamps(combined_put_stamps, tot_put, combined_get_stamps, tot_get);
#endif

#ifdef SKIP_CALCULATIONS
    printf("Skipping calculations\n");
    // Free everything used, as well as all earlier used relaxation analysis things
    free(combined_get_stamps);
    free(combined_put_stamps);
    destoy_relaxation_analysis_all(nbr_threads);
    return;
#endif
    

    uint64_t rank_error_sum = 0;
    uint64_t rank_error_max = 0;

    // Create a list of all items in the queue (in the beginning all of them)
    // TODO: For stacks we can't do this offline like this, but rather add and remove things online
    struct item_list* item_list = malloc(tot_put*sizeof(*item_list));
    for (size_t enq_ind = 0; enq_ind < tot_put; enq_ind += 1)
    {
        item_list[enq_ind].value = combined_put_stamps[enq_ind].value;
        item_list[enq_ind].next = &item_list[enq_ind+1];
    }
    item_list[tot_put-1].next = NULL;

    // The head is initially the item enqueued first
    struct item_list* head = &item_list[0];

    // For every dequeue, search the queue from the head for the dequeued item. Follow pointers to only search items not already dequeued
    for (size_t deq_ind = 0; deq_ind < tot_get; deq_ind += 1)
    {
        sval_t key = combined_get_stamps[deq_ind].value;

        uint64_t rank_error;
        if (head->value == key)
        {
            head = head->next;
            rank_error = 0;
        }
        else
        {
            rank_error = 1;
            struct item_list *current = head;
            while (current->next->value != key)
            {
                current = current->next;
                rank_error += 1;
                if (current->next == NULL)
                {
                    perror("Out of bounds on finding matching relaxation enqueue\n");
                    printf("%zu\n", deq_ind);
                    exit(-1);
                }
            }

            // current->next has the removed item, so just unlink it from the data structure
            current->next = current->next->next;
        }

        // Store rank error in get_stamps for variance calculation
        combined_get_stamps[deq_ind].value = rank_error;

        rank_error_sum += rank_error;
        if (rank_error > rank_error_max) rank_error_max = rank_error;
    }

    long double rank_error_mean = (long double) rank_error_sum / (long double) tot_get;
    if (tot_get == 0) rank_error_mean = 0.0;
    printf("mean_relaxation , %.4Lf\n", rank_error_mean);
    printf("max_relaxation , %zu\n", rank_error_max);

    // Find variance
    long double rank_error_variance = 0;
    for (size_t deq_ind; deq_ind < tot_get; deq_ind += 1)
    {
        long double off = (long double) combined_get_stamps[deq_ind].value - rank_error_mean;
        rank_error_variance += off*off;
    }
    rank_error_variance /= tot_get - 1;

    printf("variance_relaxation , %.4Lf\n", rank_error_variance);

    // Free everything used, as well as all earlier used relaxation analysis things
    free(item_list);
    free(combined_get_stamps);
    free(combined_put_stamps);
    destoy_relaxation_analysis_all(nbr_threads);
}


