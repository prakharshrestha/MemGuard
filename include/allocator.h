#include <stddef.h>
#include <stdbool.h>

#define CANARY_MAGIC 0xDEADBEEF

typedef struct {
    size_t total_allocated;
    size_t active_allocations;
    size_t largest_free_block;
    size_t corruption_events;
    size_t total_cumulative_allocated;
} allocator_stats;

void *malloc(size_t size);
void *calloc(size_t n, size_t size);
void *realloc(void *ptr, size_t size);
void free(void *ptr);

allocator_stats get_allocator_stats(void);