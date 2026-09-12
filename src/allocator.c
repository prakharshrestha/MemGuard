#include <stdint.h> 
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdbool.h>
#include <assert.h>
#include "allocator.h"
#include <stdio.h>
#include <stdlib.h>

struct block_meta {
    size_t size;
    size_t requested_size;
    struct block_meta* next;  
    bool free; 
    bool is_mmap; 
    uint32_t front_canary;
}; 

const size_t META_SIZE = sizeof(struct block_meta);
const size_t MMAP_THRESHOLD = 128 * 1024; // 128 KB
struct block_meta *free_list = NULL; 

static allocator_stats global_stats = {0, 0, 0, 0, 0};

allocator_stats get_allocator_stats(void) {
    size_t largest_free = 0;
    struct block_meta *current = free_list;
    while (current != NULL) {
        if (current->free && current->size > largest_free) {
            largest_free = current->size;
        }
        current = current->next;
    }
    global_stats.largest_free_block = largest_free;
    return global_stats;
}

static void set_canaries(struct block_meta *block, size_t requested_size) {
    block->front_canary = CANARY_MAGIC;
    block->requested_size = requested_size;
    uint32_t *rear = (uint32_t*)((char*)(block + 1) + requested_size);
    *rear = CANARY_MAGIC;
}

static bool check_canaries(struct block_meta *block) {
    if (block->front_canary != CANARY_MAGIC) {
        return false;
    }
    uint32_t *rear = (uint32_t*)((char*)(block + 1) + block->requested_size);
    if (*rear != CANARY_MAGIC) {
        return false;
    }
    return true;
}

static void handle_corruption(const char *msg) {
    fprintf(stderr, "MemGuard: Heap Corruption Detected! %s\n", msg);
    global_stats.corruption_events++;
}

struct block_meta *find_free_block(struct block_meta **last, size_t size) {
    struct block_meta *current = free_list;

    while (current != NULL && !(current->free && current->size >= size)) {
        *last = current;
        current = current->next;
    }
    return current; 
}

struct block_meta *request_space(struct block_meta *last, size_t size) {
    void *request = sbrk(size + META_SIZE);
    
    if (request == (void*)-1) {
        return NULL;
    }
    
    struct block_meta *block = (struct block_meta *)request;
    block->size = size;
    block->next = NULL;
    block->free = false;
    block->is_mmap = false;
    
    if (last) {
        last->next = block;
    }
    
    return block;
}

void split_block(struct block_meta *block, size_t size) {
   if (block->size >= size + META_SIZE + sizeof(void*) + sizeof(uint32_t)) {
      struct block_meta *new_block = (struct block_meta*)((char*)(block + 1) + size);
      new_block->size = block->size - size - META_SIZE; 
      new_block->next = block->next; 
      new_block->free = true; 
      new_block->is_mmap = false; 
      new_block->front_canary = 0; // Not strictly necessary, but good for hygiene
      
      block->size = size; 
      block->next = new_block; 
   }
}

void coalesce_block(struct block_meta *block) {
   if (block->next && block->next->free) {
      block->size += META_SIZE + block->next->size ; 
      block->next = block->next->next; 
   }

   struct block_meta *current = free_list;

   while (current && current->next != block) {
      current = current->next; 
   }

   if (current && current->free) {
      current->size += META_SIZE + block->size; 
      current->next = block->next; 
   }
}

void *malloc(size_t size) {
    struct block_meta *block; 

    if (size == 0) {
        return NULL; 
    }

    size_t needed_space = size + sizeof(uint32_t); // Space for rear canary
    size_t aligned_size = (needed_space + sizeof(void*) - 1) & ~(sizeof(void*) - 1); 

    if (aligned_size >= MMAP_THRESHOLD) {
      void *ptr = mmap(NULL, aligned_size + META_SIZE, 
                  PROT_READ | PROT_WRITE,
                  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0); 

      if (ptr == MAP_FAILED) {
         return NULL; 
      }

      block = (struct block_meta *)ptr;

      block->size = aligned_size; 
      block->next = NULL; 
      block->free = false;
      block->is_mmap = true; 
      set_canaries(block, size);
      
      global_stats.total_allocated += block->size;
      global_stats.total_cumulative_allocated += block->size;
      global_stats.active_allocations++;
      return (block + 1); 
    }
    else {
      if (!free_list) {
         block = request_space(NULL, aligned_size); 
         if (!block) { 
               return NULL; 
         }
         free_list = block; 

      } else {
         struct block_meta *last = free_list;
         block = find_free_block(&last, aligned_size); 
         if (!block) {
               block = request_space(last, aligned_size);
               if (!block) {
                  return NULL; 
               }
         } else {
               split_block(block, aligned_size);
               block->free = false; 
         }
      }
    }
    
    set_canaries(block, size);
    global_stats.total_allocated += block->size;
    global_stats.total_cumulative_allocated += block->size;
    global_stats.active_allocations++;
    
    return (block + 1); 
}

void *calloc(size_t n, size_t size) {
   if (n == 0 || size == 0) {
       return NULL;
   }
   
   if (size > SIZE_MAX / n) {
      return NULL; 
   }

   void *ptr = malloc(n * size); 
   if (ptr == NULL) {
      return NULL; 
   }

   memset(ptr, 0, n * size); 
   return ptr; 
}

void *realloc(void *ptr, size_t size) {
   if (!ptr) {
      return malloc(size); 
   }

   if (size == 0) {
      free(ptr);
      return NULL; 
   }

   struct block_meta *block = (struct block_meta*)ptr - 1;

   if (block->free) {
       handle_corruption("Double free detected in realloc!");
       return NULL;
   }
   if (!check_canaries(block)) {
       handle_corruption("Canary check failed in realloc!");
       return NULL;
   }

   size_t needed_space = size + sizeof(uint32_t);
   size_t aligned_size = (needed_space + sizeof(void*) - 1) & ~(sizeof(void*) - 1);

   if (block->size >= aligned_size) {
      set_canaries(block, size);
      return ptr;
   }

   if (!block->is_mmap && block->next && block->next->free && 
       block->size + META_SIZE + block->next->size >= aligned_size) {
      block->size += META_SIZE + block->next->size;
      block->next = block->next->next;
      split_block(block, aligned_size);
      set_canaries(block, size);
      return ptr;
   }
   
   void *new_ptr = malloc(size);
   if (!new_ptr) {
      return NULL;
   }
   
   size_t copy_size = (block->requested_size < size) ? block->requested_size : size;
   memcpy(new_ptr, ptr, copy_size);
   free(ptr);
   return new_ptr;
}

void free(void* ptr) {
   if (!ptr) {                            
      return;
   }

   struct block_meta *block = (struct block_meta*)ptr - 1;

   if (block->free) {
       handle_corruption("Double free detected in free!");
       return;
   }
   if (!check_canaries(block)) {
       handle_corruption("Canary check failed in free!");
       return;
   }

   global_stats.total_allocated -= block->size;
   global_stats.active_allocations--;

   if (block->is_mmap) {
      munmap(block, block->size + META_SIZE);
   }
   else {
      block->free = true;
      coalesce_block(block); 
   }
}
