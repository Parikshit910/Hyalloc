#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "hyalloc.h"

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
int main() {
  init();
  run_random_walk();
  hydestroy();
  return 0;
}