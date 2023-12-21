// Copyright 2021 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

// Note: This test is only for Terapool dynamic heap allocation
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
#define MIN_GROUP_FACTOR (1)
#define MAX_GROUP_FACTOR (128)  // only have 128 tiles in Terapool system

int main() {
  uint32_t core_id = mempool_get_core_id();
  uint32_t num_cores = mempool_get_core_count();

  // Initialization
  mempool_init(core_id);
  // Initialize Heap Allocators
  // mempool_dynamic_heap_alloc_init(core_id, GROUP_FACTOR);
  // Initialize synchronization variables
  mempool_barrier_init(core_id);

  if (core_id == 0) {
  	printf("Initialize\n");

  	// ---------------------------------------
  	// Interleaved Heap Allocator
  	// ---------------------------------------
  	// printf("Test1: Interleaved Heap Region.\n");
	// alloc_dump(get_alloc_l1());

	// uint32_t *array = (uint32_t *) simple_malloc(ARRAY_SIZE * sizeof(uint32_t));
	// printf("Allocated array at %08X with size %u\n", array, ARRAY_SIZE);

	// alloc_dump(get_alloc_l1());

 //    // Free
 //    simple_free(array);
 //    printf("Freed array at %08X with size %u\n", array, ARRAY_SIZE);

  	// ---------------------------------------
  	// Sequential Heap Allocator
  	// ---------------------------------------
  	// printf("Test2: Sequential Heap Region.\n");
 //  	uint32_t num_partition = mempool_get_tile_count() / GROUP_FACTOR;
	// mempool_dynamic_heap_alloc_init(core_id, GROUP_FACTOR);
 //  	for (uint32_t part_id=0; part_id<num_partition; ++part_id){
 //  	  // uint32_t part_id = 1;
 //      // printf("Test partiton allocator %u: \n", part_id);

 //      alloc_t *dynamic_heap_alloc = get_dynamic_heap_alloc(part_id);

 //      // print out
 //      alloc_dump(dynamic_heap_alloc);

 //      // Malloc uint32_t array of size 16 (40 bytes)
 //      uint32_t *part_array = (uint32_t *)domain_malloc(dynamic_heap_alloc, ARRAY_SIZE * 4);
 //      // printf("Allocated array at %08X with size %u\n", part_array, ARRAY_SIZE);

 //      // Print out allocator
 //      alloc_dump(dynamic_heap_alloc);

 //      // Copy data
 //      for (uint32_t i=0; i<ARRAY_SIZE; ++i){
 //      	part_array[i] = i;
 //      }
 //      // Free array
 //      domain_free(dynamic_heap_alloc, part_array);
 //      // printf("Freed array at %08X with size %u\n", part_array, ARRAY_SIZE);

 //      // Print out allocator
 //      alloc_dump(dynamic_heap_alloc);
 //    }

 //    // Free the Dynamic Allocator
 //    free_dynamic_heap_alloc();

    // --------------------------------------------
    // Runtime Partition Selection
    // --------------------------------------------

  	uint32_t part_id = 0;
  	for (uint32_t group_factor=1; group_factor<=128; group_factor*=2){
  		// 1. Init dynamic heap allocator
  		partition_config(group_factor);
    	mempool_dynamic_heap_alloc_init(core_id, group_factor);

    	// 2. Set which partition write to. We will write to partition_1 for this test
    	uint32_t num_partition = mempool_get_tile_count() / group_factor;
    	if (num_partition == 1){
    		part_id = 0;
    	}
    	else {
    		part_id = num_partition-2;  // set to allocate in the penultimate partition
    	}

    	// 3. Get the allocator and starting address to this region
    	alloc_t *dynamic_heap_alloc = get_dynamic_heap_alloc(part_id);
    	alloc_dump(dynamic_heap_alloc);
    	uint32_t *part_array = (uint32_t *)domain_malloc(dynamic_heap_alloc, ARRAY_SIZE * group_factor * 4); // ARRAY_SIZE = 2*#BanksPerTile
    	
    	// 4. Move data
    	for (uint32_t i=0; i<ARRAY_SIZE*group_factor; ++i){
  	  		part_array[i] = i;
  		}

  		// 5. Free array
  		domain_free(dynamic_heap_alloc, part_array);

  		// 6. Free dynamic allocator
  		free_dynamic_heap_alloc();

  	}

  	// Corner case: group_factor = 0 (default partition)
    // 1. Init dynamic heap allocator
 //    group_factor = 0;
 //    partition_config(128);
 //    mempool_dynamic_heap_alloc_init(core_id, group_factor);

 //    // 2. Set which partition write to. We will write to partition_1 for this test
 //    part_id = 1;

 //    // 3. Get the allocator and starting address to this region
 //    alloc_t *dynamic_heap_alloc_1 = get_dynamic_heap_alloc(part_id);
 //    alloc_dump(dynamic_heap_alloc_1);

 //    uint32_t *part_array_1 = (uint32_t *)domain_malloc(dynamic_heap_alloc_1, ARRAY_SIZE * 4);

	// // 4. Move data
 //  	for (uint32_t i=0; i<ARRAY_SIZE; ++i){
 //  	  part_array_1[i] = i;
 //  	}

 //  	// 5. Free array
 //  	domain_free(dynamic_heap_alloc_1, part_array_1);

 //  	// 6. Free dynamic allocator
 //  	free_dynamic_heap_alloc();

    printf("Done!\n");
  }

  mempool_barrier(num_cores);
  return 0;


}