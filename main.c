#include <stdio.h>
#include "hyalloc.h"
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
void test_threshold_boundary() {
    void* ptr = hymalloc(THRESHOLD);

    /* LOGIC CHECK: 
       Because size is NOT > THRESHOLD, this should be an Implicit block.
       It will NOT have a Block struct header.
    */
    
    if (ptr != NULL) {
        printf("Successfully allocated 32 bytes from the Implicit pool.\n");
        // We don't check metadata here because there isn't any!
        printf("RESULT: PASS\n\n");
    } else {
        printf("RESULT: FAIL (Small pool might be full)\n\n");
    }
}
void test_heap_overflow(){
    int count = CHUNKSIZE/THRESHOLD + 1;
    void* ptr;
    for (int i = 0; i < count; i++)    
        ptr = hymalloc(32);
    
    if (ptr == NULL)
    {
        printf("TEST PASS\n");
    }else{
        printf("TEST FAILED\n");
    }
    
}
void test_free_reuse(){
    void* ptr1 = hymalloc(100);
    printf("First Alloc:  %p\n", ptr1); // No & here!
    
    hyfree(ptr1, 100); // Make sure size matches your logic
    
    void* ptr2 = hymalloc(100);
    printf("Second Alloc: %p\n", ptr2); // No & here!

    if (ptr1 == ptr2) {
        printf("RESULT: PASS (Memory Recycled)\n");
    } else {
        printf("RESULT: FAIL (New block used instead of old one)\n");
    }
}
void test_explicit_coalescing() {
    printf("--- Running: Explicit Coalesce Test ---\n");
    void* ptrA = hymalloc(100);
    void* ptrB = hymalloc(100);
    void* ptrC = hymalloc(100); // ptrC acts as a "buffer" to the rest of the heap

    printf("Block A: %p, Block B: %p\n", ptrA, ptrB);

    // 2. Free the first two
    hyfree(ptrA, 100);
    hyfree(ptrB, 100);

    // 3. Request a block larger than 100 but smaller than 200
    // If A and B merged, ptrD should equal ptrA.
    void* ptrD = hymalloc(180);
    printf("Block D (180 bytes): %p\n", ptrD);

    if (ptrD == ptrA) {
        printf("RESULT: PASS (Adjacent blocks merged successfully)\n");
    } else {
        printf("RESULT: FAIL (Blocks stayed fragmented)\n");
    }
}
int main(int argc, char const *argv[])
{
    init();
    test_explicit_coalescing();
    return 0;
}
