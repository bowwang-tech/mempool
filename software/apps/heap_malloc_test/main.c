// Copyright 2021 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

// Author: Bowen Wang

#include <stdint.h>
#include <string.h>

// Data stored in L2
#include "data.h"

#include "alloc.h"
#include "encoding.h"
#include "printf.h"
#include "runtime.h"
#include "synchronization.h"

#define GROUP_FACTOR (8)

int main() {
  uint32_t core_id = mempool_get_core_id();
  uint32_t num_cores = mempool_get_core_count();

  // Initialization
  mempool_init(core_id);
  // Initialize Heap Allocators
  mempool_dynamic_heap_alloc_init(core_id, GROUP_FACTOR);
  // Initialize synchronization variables
  mempool_barrier_init(core_id);

  if (core_id == 0) {
  	printf("Initialize\n");

  	// ---------------------------------------
  	// Interleaved Heap Allocator
  	// ---------------------------------------
  	printf("Test1: Interleaved Heap Region.\n");
	alloc_dump(get_alloc_l1());

	uint32_t *array = (uint32_t *) simple_malloc(ARRAY_SIZE * sizeof(uint32_t));
	printf("Allocated array at %08X with size %u\n", array, ARRAY_SIZE);

	alloc_dump(get_alloc_l1());

    // Free
    simple_free(array);
    printf("Freed array at %08X with size %u\n", array, ARRAY_SIZE);

  	// ---------------------------------------
  	// Sequential Heap Allocator
  	// ---------------------------------------
  	printf("Test2: Sequential Heap Region.\n");
  	uint32_t num_partition = mempool_get_tile_count() / GROUP_FACTOR;

  	for (uint32_t part_id=0; part_id<num_partition; ++part_id){
      printf("Test partiton allocator %u: \n", part_id);

      alloc_t *dynamic_heap_alloc = get_dynamic_heap_alloc(part_id);

      // print out
      alloc_dump(dynamic_heap_alloc);

      // Malloc uint32_t array of size 16 (40 bytes)
      uint32_t *part_array = (uint32_t *)domain_malloc(dynamic_heap_alloc, ARRAY_SIZE * 4);
      printf("Allocated array at %08X with size %u\n", part_array, ARRAY_SIZE);

      // Print out allocator
      alloc_dump(dynamic_heap_alloc);

      // Free array
      domain_free(dynamic_heap_alloc, part_array);
      printf("Freed array at %08X with size %u\n", part_array, ARRAY_SIZE);

      // Print out allocator
      alloc_dump(dynamic_heap_alloc);
    }

    // Free the Dynamic Allocator
    free_dynamic_heap_alloc();

    printf("Done!\n");
  }

  mempool_barrier(num_cores);
  return 0;


}