# HyAlloc: A Hybrid C Memory Allocator

A custom memory allocation library implemented from scratch in C, featuring a dual-tier strategy for small and large memory requests. This project explores the "behind the scenes" of the standard malloc and free functions.
--
## Key Features
1. Hybrid Allocation Strategy: Uses different mechanisms based on requested size to optimize speed and minimize fragmentation.

2. Implicit Free List: Handles small blocks (≤32 bytes) with zero metadata overhead using a pre-sliced segregated pool.

3. Explicit Doubly-Linked List: Manages large blocks with a doubly-linked list, allowing for complex operations like coalescing.

4. Automated Fragmentation Control: Includes a *coalesce_everything()* pass to merge adjacent free blocks, followed by an Insertion Sort to maintain free-list order for faster searching.

5. Memory Alignment: All allocations are 8-byte aligned to ensure hardware compatibility.

## Deep Dive
### The Small Pool (Implicit List)
Requests ≤THRESHOLD bytes are routed to the imp_heap. I designed this as a **Singly Linked Segregated List**. Instead of using headers that waste space, the "next" pointer is stored directly inside the free memory chunk itself. This ensures that a 32-byte request uses exactly 32 bytes of the heap.
### The Large Pool (Explicit List)
For requests >THRESHOLD bytes are routed to exp_heap. This is a **Doubly Linked Size Ordered List** (smallest block will be head).  Each  Block has metadata 
```c
typedef struct Block {
    size_t size;
    bool is_Free;
    struct Block* next;
    struct Block* prev;
} Block;
```
### Fragmentation Management
When the allocator cannot find a suitable block, it triggers *coalesce_everything()*. This function:
1. Physically traverses the heap to find adjacent free blocks.

2. Merges them into a single larger block.

3. Re-sorts the free list using an Insertion Sort (O(n) in nearly-sorted cases) to maintain size order, facilitating a "Best-Fit" allocation style.
   
## Some Test I Performed
1. ### test_metadata_alignment
    ```c
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
    ```
    **Result**
    ```bash
    Requested: 33 bytes
    Actual Block Size (Aligned): 40
    RESULT: PASS
    ```
2. ### test_threshold_boundary
   ```c
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
   ```
   **Result**
   ```bash
    Successfully allocated 32 bytes from the Implicit pool.
    RESULT: PASS
   ```
3. ### test_heap_overflow
    ```c
    void test_heap_overflow(){
    int count = CHUNKSIZE/THRESHOLD + 1;
    void* ptr;
    for (int i = 0; i < count; i++)
        ptr = hymalloc(32);
    if (ptr == NULL)
    {
        printf("TEST PASS\n");
    }else
        printf("TEST FAILED\n");
    }    
    ```
    **Result**
    ```bash
    TEST PASS
    ```
4. ### test_free_reuse(the most importent)
   ```c
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
   ```
   **Result**
   ```bash
    First Alloc:  0x403080
    Second Alloc: 0x403080
    RESULT: PASS (Memory Recycled)
   ```
5. ### test_explicit_coalescing
   ```c
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
   ```
   **Result**
   ```bash
    --- Running: Explicit Coalesce Test ---
    Block A: 0x403080, Block B: 0x403108
    Block D (180 bytes): 0x403080
    RESULT: PASS (Adjacent blocks merged successfully)
   ```
## Limitations
1. The main limitation is that it uses *Static Memmory*. We use 2 lists,, implicit and explicit list of size **CHUNKSIZE** so we are limited to that. I am working on dynamic memmory version so you can expect it in future. 
2. This is also not *thread safe* so it can giive rise to **race** conditions(I will be solving it in dynamic alloc).
3. Allocation times is slow **(O(n))** , since we have to travel linked list till we find the suitable block.