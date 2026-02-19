#ifndef HYALLOC_H
#define HYALLOC_H
#define MAX_MMAP_CHUNKS 16384
#include <stddef.h>
#include <stdbool.h>

#define THRESHOLD 32
#define CHUNKSIZE (1024 * 1024) 


typedef struct Block {
    size_t size;
    bool is_Free;
    struct Block* next;
    struct Block* prev;
} Block;


struct mmap_info {
    size_t size;
    void* pointer;
};

extern void* exp_heap;
extern void* imp_heap;
extern Block* head;
extern Block* main_blk;
extern void* small_free_head;
extern int num_of_mmap;
extern int num_of_imp_mmap;


int init(void);

void* hymalloc(size_t size);


void hyfree(void* ptr, int size);


void* hycalloc(size_t size);


void* hyrealloc(void* ptr, size_t old_size, size_t new_size);


int hydestroy(void);

/* --- Internal Logic Helpers (Optional to expose) --- */
void coalesce_everything(void);
int check_range(Block* ptr);

#endif /* HYALLOC_H */