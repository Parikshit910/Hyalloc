#ifndef HYALLOC_H
#define HYALLOC_H

#include <stddef.h>
#include <stdbool.h>


#define THRESHOLD 32
#define CHUNKSIZE (1<<12)

typedef struct Block {
    size_t size;
    bool is_Free;
    struct Block* next;
    struct Block* prev;
} Block;


int init(void);

void* hymalloc(size_t size);
void hyfree(void* ptr, int size);
void* hycalloc(size_t size);
void* hyrealloc(void* ptr, size_t old_size, size_t new_size);

#define ALIGN(size) (((size) + 7) & ~7)

#endif