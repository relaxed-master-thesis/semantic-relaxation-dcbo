/*
 *   File: skiplist-lock.c
 *   Author: Vincent Gramoli <vincent.gramoli@sydney.edu.au>,
 *  	     Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>
 *   Description:
 *   skiplist-lock.c is part of ASCYLIB
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

#include "queue-lock.h"
#include "utils.h"

__thread ssmem_allocator_t* alloc;


queue_node_t*
queue_new_node(skey_t key, sval_t val, queue_node_t* next)
{
#if GC == 1
  queue_node_t* node = ssmem_alloc(alloc, sizeof(*node));
#else
  queue_node_t* node = ssalloc(sizeof(*node));
#endif

  node->key = key;
  node->val = val;
  node->next = next;

#ifdef __tile__
  MEM_BARRIER;
#endif

  return node;
}

void
queue_delete_node(queue_node_t *n)
{
#if GC == 1
  ssmem_free(alloc, (void*) n);
#endif
}

queue_t*
queue_new(int thread_id)
{
  queue_t *set;
  ssalloc_init();
  #if GC == 1
  if (alloc == NULL)
  {
  	alloc = (ssmem_allocator_t*) malloc(sizeof(ssmem_allocator_t));
  	assert(alloc != NULL);
  	ssmem_alloc_init_fs_size(alloc, SSMEM_DEFAULT_MEM_SIZE, SSMEM_GC_FREE_SET_SIZE, thread_id);
  }
  #endif

  if ((set = (queue_t*) ssalloc_aligned(CACHE_LINE_SIZE, sizeof(queue_t))) == NULL)
    {
      perror("malloc");
      exit(1);
    }

  queue_node_t* node = queue_new_node(0, 0, NULL);
  set->head = node;
  set->tail = node;
  INIT_LOCK(&(set->head_lock));
  INIT_LOCK(&(set->tail_lock));

  return set;
}

void
queue_delete(queue_t *set)
{
  printf("queue_delete - implement me\n");
}

int queue_size(queue_t *set)
{
  int size = 0;
  queue_node_t *node;

  /* We have at least 2 elements */
  node = set->head;
  while (node->next != NULL)
    {
      size++;
      node = node->next;
    }
  return size;
}

queue_t* queue_register(queue_t *set, int thread_id)
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