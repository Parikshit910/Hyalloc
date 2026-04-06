#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "hyalloc.h"
#define _POSIX_C_SOURCE 199309L
#define ITERATIONS 10000
#define MAX_POINTERS 500
#define MAX_ALLOC_SIZE 8192 // 8KB
void test_metadata_alignment() {
    size_t request_size = 33;
    int* arr = (int*)(hymalloc(request_size));
    
    // Move pointer back to the start of the hidden struct
    Block* check = (Block*)((char*)arr - sizeof(Block));
    
    printf("Requested: %zu bytes\n", request_size);
    printf("Actual Block Size (Aligned): %zu\n", check->size);
    
    if (check->size >= request_size && check->size % 8 == 0) {
        printf("RESULT: PASS\n\n");
    } else {
        printf("RESULT: FAIL\n\n");
    }
}
void test_imp_island_jump() {
    init();
    
    // 1. Calculate how many blocks fit in exactly one CHUNKSIZE
    int blocks_per_chunk = CHUNKSIZE / THRESHOLD;
    void* last_ptr_in_chunk = NULL;
    void* first_ptr_new_chunk = NULL;

    printf("Filling first chunk with %d blocks...\n", blocks_per_chunk);

    // 2. Exhaust the first chunk
    for (int i = 0; i < blocks_per_chunk; i++) {
        last_ptr_in_chunk = hymalloc(32);
        if (last_ptr_in_chunk == NULL) {
            printf("FAIL: Could not even fill the first chunk!\n");
            return;
        }
    }

    // 3. This allocation MUST trigger inc_imp_heap()
    printf("Triggering inc_imp_heap()...\n");
    first_ptr_new_chunk = hymalloc(32);

    // 4. Verification Logic
    if (first_ptr_new_chunk == NULL) {
        printf("FAIL: inc_imp_heap failed to return memory.\n");
    } else if ((char*)first_ptr_new_chunk < (char*)last_ptr_in_chunk + CHUNKSIZE && 
               (char*)first_ptr_new_chunk > (char*)last_ptr_in_chunk - CHUNKSIZE) {
        // If the new pointer is within 1MB of the old one, it might just be the same chunk.
        // In mmap, chunks are usually far apart.
        printf("WARNING: New block is very close to old block. Check mmap logic.\n");
    } else {
        printf("SUCCESS: Island jump successful!\n");
        printf("Last block (Chunk 1): %p\n", last_ptr_in_chunk);
        printf("First block (Chunk 2): %p\n", first_ptr_new_chunk);
    }

    hydestroy();
}
struct alloc_info
{
    void* ptr;
    int size;
} info[1000];

void nullify(void){
    for (int i = 0; i < 1000; i++)
    {
        info[i].ptr  = NULL;
        info[i].size = 0;
    }
    
}
void run_random_walk() {
    /*walk*/
    int iterations = 0;
    for (int i = 0; i < 10000; i++)
    {   iterations++;
        int index = rand() % 1000;
        if (info[index].ptr == NULL)
        {   
            int called_size = rand() % 513;

            void* called_ptr = hymalloc(called_size);
            info[index].ptr = called_ptr;
            info[index].size = called_size;

        }
        else{
            int action = rand() % 2;
            if (action == 0)
            {   
                hyfree(info[index].ptr, info[index].size);
                info[index].ptr = NULL;
                info[index].size = 0;
            }
            
        }
    }
    printf("Random walk completed with %d iterations.\n", iterations);
    for (int i = 0; i < 1000; i++)
    {
        
        if (info[i].ptr != NULL)
        {
            hyfree(info[i].ptr, info[i].size);
        }
        
    }
    printf("all memory freed\n");
    hydestroy();
}

#define SMALL_COUNT 5000
#define LARGE_COUNT 100

void test_memory_limits() {
    printf("--- Starting Memory Blowup & Fragmentation Test ---\n");

    // 1. Stress the Implicit List (Segregated 32-byte blocks)
    // This will force inc_imp_heap to be called multiple times.
    void* small_ptrs[SMALL_COUNT];
    printf("Allocating %d small blocks...\n", SMALL_COUNT);
    for (int i = 0; i < SMALL_COUNT; i++) {
        small_ptrs[i] = hymalloc(THRESHOLD - 10); 
        if (!small_ptrs[i]) {
            printf("CRITICAL: Small allocation failed at index %d\n", i);
            return;
        }
        memset(small_ptrs[i], 0, THRESHOLD - 10); // Ensure memory is writable
    }
    // 2. Fragment the Explicit List
    // Allocate large blocks, then free the even-numbered ones.
     void* large_ptrs[LARGE_COUNT];
    printf("Creating fragmentation in explicit list...\n");
    for (int i = 0; i < LARGE_COUNT; i++) {
        large_ptrs[i] = hymalloc(2048); // 2KB blocks
    }

    // Free every second block to create gaps
    for (int i = 0; i < LARGE_COUNT; i += 2) {
        hyfree(large_ptrs[i], 2048);
    }

    // 3. Test Coalescing & Search
    // Request a 4KB block. Since we freed 2KB blocks, 
    // the allocator MUST coalesce adjacent 2KB gaps to fulfill this.
    printf("Testing coalescing logic (requesting 4KB)...\n");
    void* mid_size = hymalloc(4096);
    if (mid_size) {
        printf("SUCCESS: Coalesced blocks used.\n");
        hyfree(mid_size, 4096);
    } else {
        printf("FAILURE: Could not coalesce blocks.\n");
    }

    // 4. Test Expansion (The Wilderness)
    // Request something huge to force inc_exp_heap
    printf("Requesting massive block to force heap expansion...\n");
    void* massive = hymalloc(CHUNKSIZE * 2);
    if (massive) {
        printf("SUCCESS: Heap expanded via mmap.\n");
        hyfree(massive, CHUNKSIZE * 2);
    }

    // 5. Cleanup
    printf("Cleaning up remaining allocations...\n");
    for (int i = 1; i < LARGE_COUNT; i += 2) {
        hyfree(large_ptrs[i], 2048);
    }
    for (int i = 0; i < SMALL_COUNT; i++) {
        hyfree(small_ptrs[i], THRESHOLD - 10);
    }
}
#define NUM_OPERATIONS 100000
#define TEST_SIZE 24 // Fits in your THRESHOLD 32

double get_time_diff(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1E9;
}

void run_hymalloc_bench() {
    struct timespec start, end;
    void* ptrs[NUM_OPERATIONS];

    init();
    printf("Starting hymalloc benchmark (%d ops)...\n", NUM_OPERATIONS);
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < NUM_OPERATIONS; i++) {
        ptrs[i] = hymalloc(TEST_SIZE);
    }
    for (int i = 0; i < NUM_OPERATIONS; i++) {
        hyfree(ptrs[i], TEST_SIZE);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    printf("hymalloc time: %f seconds\n", get_time_diff(start, end));
    hydestroy();
}

void run_std_malloc_bench() {
    struct timespec start, end;
    void* ptrs[NUM_OPERATIONS];

    printf("Starting standard malloc benchmark (%d ops)...\n", NUM_OPERATIONS);
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < NUM_OPERATIONS; i++) {
        ptrs[i] = malloc(TEST_SIZE);
    }
    for (int i = 0; i < NUM_OPERATIONS; i++) {
        free(ptrs[i]);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    printf("std malloc time: %f seconds\n", get_time_diff(start, end));
}

#define TOTAL_OPS 100000
#define SMALL_SIZE (THRESHOLD - 8) // Hits the implicit list
#define LARGE_MIN 128              // Hits the explicit list
#define LARGE_MAX 2048

void run_hybrid_bench() {
    struct timespec start, end;
    void* ptrs[TOTAL_OPS];
    size_t sizes[TOTAL_OPS];

    init();
    srand(time(NULL));

    printf("Starting Hybrid Benchmark (70%% Small / 30%% Large)...\n");
    
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < TOTAL_OPS; i++) {
        // Generate a weight between 0 and 99
        int weight = rand() % 100;

        if (weight < 70) {
            // 70% Small calls
            sizes[i] = SMALL_SIZE;
        } else {
            // 30% Large calls
            sizes[i] = LARGE_MIN + (rand() % (LARGE_MAX - LARGE_MIN));
        }

        ptrs[i] = hymalloc(sizes[i]);
        
        // Safety: Touch memory to ensure it's mapped
        if (ptrs[i]) *((char*)ptrs[i]) = 1; 
    }

    // Free everything
    for (int i = 0; i < TOTAL_OPS; i++) {
        if (ptrs[i]) {
            hyfree(ptrs[i], sizes[i]);
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    
    double time_taken = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1E9;
    printf("Hybrid Benchmark Time: %f seconds\n", time_taken);
    printf("Ops per second: %.0f\n", TOTAL_OPS / time_taken);

    hydestroy();
    printf("Final Mmap Count: %d\n", num_of_mmap);
}

