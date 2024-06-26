#ifndef RELAXATION_ANALYSIS_COUNTER_H
#define RELAXATION_ANALYSIS_COUNTER_H

#include "lock_if.h"
#include "common.h"

#define REL_NSIZE 1024
typedef struct relaxation_list {
    struct relaxation_list* next;
    size_t counts;
    uint32_t error[REL_NSIZE];
} relaxation_list_t;

static ptlock_t relaxation_analysis_lock;
static uint64_t strict_counter;
static relaxation_list_t* rel_start;
static relaxation_list_t* rel_tail;

static inline void inc_relaxed_count()
{
    strict_counter += 1;
}

static inline void dec_relaxed_count()
{
    assert(strict_counter > 0);
    strict_counter -= 1;
}

// Documents a relaxed return. Must hold lock during the duration of this and the counter read
static inline void add_relaxed_count(uint32_t count)
{
    if (rel_tail->counts == REL_NSIZE)
    {
        relaxation_list_t* next = (relaxation_list_t*) calloc(1, sizeof(relaxation_list_t));
        rel_tail->next = next;
        rel_tail = next;
    }
    // Just use the absolute error as the relaxation error
    uint32_t error = strict_counter > count ? (strict_counter - count) : (count - strict_counter);
    rel_tail->error[rel_tail->counts] = error;
    rel_tail->counts += 1;
}

void lock_relaxation_lists()
{
    LOCK(&relaxation_analysis_lock);
}


void unlock_relaxation_lists()
{
    UNLOCK(&relaxation_analysis_lock);
}

static inline void init_relaxation_analysis()
{
    INIT_LOCK(&relaxation_analysis_lock);
    rel_start = rel_tail = (relaxation_list_t*) calloc(1, sizeof(relaxation_list_t));;
}

void print_relaxation_measurements()
{

    // Long double used to not run into a max of ints. Maybe gives even worse results?
    uint64_t max;
    uint64_t samples;
    long double sum, mean, variance;

    lock_relaxation_lists();


    // Find mean and maximum
    sum = 0;
    max = 0;
    samples = 0;
    relaxation_list_t* current_node = rel_start;
    size_t current_count = 0;
    while (1)
    {
        if (current_count == REL_NSIZE)
        {
            current_count = 0;
            current_node = current_node->next;
        }
        if (current_node == NULL || current_count == current_node->counts) break;

        uint64_t error = current_node->error[current_count];
        current_count += 1;
        samples += 1;

        sum += error;

        if (error > max) max = error;
    }

    mean = sum / (double) samples;
    printf("mean_relaxation , %.4Lf\n", mean);
    printf("max_relaxation , %zu\n", max);
    
    sum = 0;

    current_node = rel_start;
    current_count = 0;
    while (1)
    {
        if (current_count == REL_NSIZE)
        {
            current_count = 0;
            current_node = current_node->next;
        }
        if (current_node == NULL || current_count == current_node->counts) break;
        long double error = current_node->error[current_count];
        current_count += 1;
        sum += (error-mean) * (error-mean);
    }

    variance = sum / samples;
    printf("variance_relaxation , %.4Lf\n", variance);

#ifdef SAVE_FULL
    // Print all individually
    printf("relaxation_distances , ");
    current_node = rel_start;
    current_count = 0;
    while (1)
    {
        if (current_count == REL_NSIZE)
        {
            current_count = 0;
            current_node = current_node->next;
        }
        if (current_node == NULL || current_count == current_node->counts) break;
        uint64_t error = current_node->error[current_count];
        current_count += 1;

        printf("%zu", error);
        if (current_count == REL_NSIZE)
        {
            current_count = 0;
            current_node = current_node->next;
        }
        if (current_node == NULL || current_count == current_node->counts) {
            printf("\n");
            break;
        }
        else
        {
            printf(", ");
        }
    }
#endif

    unlock_relaxation_lists();
}

#endif
