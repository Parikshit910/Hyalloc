#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "hyalloc.h"
/* #define THRESHOLD 32 */
/*when requested size is less than THRESHOLD bytes the block will be fetched from simple segregated list of size THRESHOLD bytes,
 and when size is greater than THRESHOLD byte it will get from explicit size order list*/
/* #define CHUNKSIZE (1<<12) */
static char exp_heap[CHUNKSIZE];   // the heap

static char imp_heap[CHUNKSIZE];   // the heap
#define ALIGN(size) (((size) + 7) & ~7)

/* typedef struct Block
{
  size_t size;
  bool is_Free;
  struct Block* next;
  struct Block* prev;
} Block; */
Block* head;
Block* main_blk;
void* small_free_head;

#define GET_SIZE(p) (p->size)
#define GET_ALLOC(p) (p->is_Free)

int init_exp(void){
  main_blk = (Block*)exp_heap;
  main_blk->size = CHUNKSIZE - THRESHOLD;
  main_blk->is_Free = true;
  main_blk->next = main_blk;
  main_blk->prev = main_blk;
  head = main_blk;
  return 0;
}
/*for my implicit list i have use a singly linked simple segregated list, i have sliced
my whole static memory into 128 block each of size THRESHOLD bytes and each block holds address of next block
therefore no overhead is created, eg: say first block is at address 0 then at address 0 pointer to address THRESHOLD is saved,
at address THRESHOLD address of 64 is saved and it continues till address 4064 as it store NULL since it is last block*/
int init_imp(void){
  int block_num = 0;
  small_free_head = imp_heap;
  for(int i = 0;  i < CHUNKSIZE - THRESHOLD; i+= THRESHOLD)
  {
    *(void**)small_free_head = ((char*)imp_heap + (block_num+THRESHOLD));
     small_free_head = ((char*)imp_heap + (block_num+THRESHOLD));
    block_num += THRESHOLD;
  };
  *(void**)small_free_head = NULL;
  small_free_head = imp_heap;
  return 0;}
int init(void){
  init_imp();
  init_exp();
  return 0;
}
/*when we couldnt find block big enough then before asking to wilderness we coalsce whole free free list and the way we do it is:
  we look at head and see next block in memmory (here we are seeing neighbor in terms of memmory not through linked list) and if its free  we combine  them and 
  we continue, this might feel slow but this keeps shrinking linked list actively so it wont be that slow*/
void coalesce_everything(){
    Block* curr = head;
    void* heap_end = (char*)exp_heap + CHUNKSIZE;
    do
    {
      size_t jump = curr->size + sizeof(Block);
      Block* next_blk = (Block*)((char*)curr + jump);
      while (next_blk->is_Free && next_blk->next != next_blk && (void*)next_blk <  heap_end)
      {
        (next_blk->next)->prev = (next_blk->prev);
        (next_blk->prev)->next = (next_blk->next);
        curr->size += next_blk->size + sizeof(Block);
        size_t jump = curr->size + sizeof(Block);
        next_blk = (Block*)((char*)curr + jump);
      }
      curr =  curr->next;
    } while (curr != head);
    
}
void* return_req_ptr(Block* curr, size_t size, size_t  total_needed){
    do
    {
      if ((GET_SIZE(curr) >= size) && (curr->is_Free))
      {
        if (((curr->size) - size) < (THRESHOLD + sizeof(Block) + 1))
        {
          curr->is_Free = false;
          return (void *)(curr + 1);
        }
        else{
          if (((curr->size)-size) > size)
          {
            Block* old_nxt = curr->next;
            size_t oldsize= curr->size;
            Block* remainder = (Block*)((char*)curr + total_needed);
            curr->is_Free = false;
            curr->next = remainder;
            curr->size = size;
            remainder->is_Free = true;
            remainder->next = old_nxt;
            remainder->prev = curr;
            remainder->size = oldsize-size - sizeof(Block);
            return (void *)(curr + 1);
          }
          else{
            Block* old_nxt = curr->next;
            size_t curr_total_phys = curr->size + sizeof(Block);
            size_t front_part_size = curr_total_phys - total_needed;
            Block* remainder = (Block*)((char*)curr + front_part_size);
            curr->is_Free = true;
            curr->next = remainder;
            curr->size = front_part_size - sizeof(Block);
            remainder->is_Free = false;
            remainder->next = old_nxt;
            remainder->prev = curr;
            remainder->size = size;
            return (void *)(remainder + 1);
          }
        }
      }
      
    } while(GET_SIZE(curr) < GET_SIZE(curr->next));
    return NULL;
}

void* hymalloc(size_t size){

  Block* curr = head;
  Block* curr1 = head;

  if (size > THRESHOLD)
  {  
    size  =  ALIGN(size);
      size_t total_needed = sizeof(Block) + size;
    /*main_blk is moved from start to 32 bytes + requested size , the head is pointed towards new block if its lower than */
    if (head == main_blk)
    {
      main_blk->size = main_blk->size - total_needed;
      main_blk = (Block*)((char*)head + total_needed);
      Block* block1 = head;
      block1->is_Free = false;
      block1->next = block1;
      block1->prev = block1;
      block1->size = size;
      main_blk->next = main_blk;
      main_blk->prev = main_blk;
      return (void *)(block1 + 1);
    }
    Block* old_curr = curr;
    void* correct_ptr = return_req_ptr(curr, size,  total_needed);
    if (correct_ptr != NULL)
    {
      return correct_ptr;
    }
    coalesce_everything();
    /*now we have to remember after coalesce evrything over linked list is no more ordered by size so we use INSERTION SORT,
    now it is slow(time complexity is O(n^2)) but our linked list isnt too messed up its still partially sorted and in such conditions 
    INSERTION sort can be very efficient and its also easy to maintain*/
    Block* p1 = head;
    Block* p2 = head->next;
    while (p2 != head)
    {
      Block* next_blk = p2->next;
      Block* next_p2 = p2->next;
      while (p1 != head->prev)
      {
        if (p1->size > p2->size)
        {
          p1->next = next_blk;
          next_blk->prev = p1;
          next_blk = p1;
          p1 = p1->prev; 
        }
        else{
          p2->next = next_blk;
          p2->prev = p1;
          p1->next = p2;
          next_blk->prev = p2;
          break;
        }
      }
      p2 = next_p2;
      p1 = next_p2->prev;
      
    }
    correct_ptr = return_req_ptr(head, size,  total_needed);
    if (correct_ptr != NULL)
    {
      return correct_ptr;
    }

    /*when we couldnt find free blk in free list we need to create new blk and get memory from static memory main_blk and
    then using do-while loop we find its perfect place in the free list */
    Block* req_blk = main_blk;
    size_t old_size = main_blk->size;
    req_blk->is_Free = false;
    req_blk->size = size;
    main_blk = (Block*)((char*)req_blk + total_needed);
    main_blk->is_Free = true;
    main_blk->next = main_blk;
    main_blk->prev = main_blk;
    main_blk->size = old_size - total_needed;
    do
    {
      if((curr1->size) >= (req_blk->size)){
        Block* prev_blk = curr1->prev;
        prev_blk->next = req_blk;
        req_blk->prev = prev_blk;
        req_blk->next = curr1;
        curr1->prev = req_blk;
        return (void *)(req_blk+1);
      }
    } while (GET_SIZE(curr1) < GET_SIZE(curr1->next));
  }
  if (small_free_head == NULL)
  {
    return NULL;
  }else{
    void* to_be_return = small_free_head;
    small_free_head = *(void**)small_free_head;
    return to_be_return;
  }
  
}
/*to free implicit block we simply store the current head's data in the block that is to be free and points the 
head to this new block*/
void hyimpfree(void* ptr){
  *(void**)ptr = small_free_head;
  small_free_head = ptr;
}
void hyexpfree(Block* ptr){
  ptr->is_Free  = true;
}
void hyfree(void* ptr, int size){
  if (ptr == NULL)
  {
    return;
  }
  
  if (size<=THRESHOLD)
  {
    hyimpfree(ptr);
  }
  else{
    Block* blk_ptr = (Block*)((char*)ptr - sizeof(Block));
    hyexpfree(blk_ptr);
  }
}
void* hycalloc(size_t size){
  void* ptr = hymalloc(size);
  if (ptr != NULL)
  {
  if (size > THRESHOLD)
  {
    size = ALIGN(size);
    for (size_t i = 0; i < size; i++)
    {
      ((char*)ptr)[i] = 0;
    }
    return ptr;
  }
  for (size_t i = 0; i < size; i++)
    {
      ((char*)ptr)[i] = 0;
    }
    return ptr;
}
  return NULL;}
void* hyrealloc(void* ptr, size_t old_size, size_t new_size) {
    if (ptr == NULL) {
        return hymalloc(new_size);
    }
    if (new_size == 0) {
        hyfree(ptr, old_size);
        return NULL;
    }
    if (ALIGN(new_size) <= ALIGN(old_size)) {
        return ptr; 
    }
    void* new_ptr = hymalloc(new_size);
    if (new_ptr == NULL) {
        return NULL; // Allocation failed
    }
    char* src = (char*)ptr;
    char* dest = (char*)new_ptr;
    for (size_t i = 0; i < old_size; i++) {
        dest[i] = src[i];
    }
    hyfree(ptr, old_size);

    return new_ptr;
}




