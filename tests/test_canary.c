#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "allocator.h"

int main() {
    printf("Running canary tests...\n");
    
    allocator_stats initial_stats = get_allocator_stats();
    size_t start_corruption = initial_stats.corruption_events;

    // Normal alloc/free
    void *ptr = malloc(100);
    assert(ptr != NULL);
    free(ptr);
    
    allocator_stats stats = get_allocator_stats();
    assert(stats.corruption_events == start_corruption);

    // Buffer overflow (overwrite rear canary)
    ptr = malloc(50);
    char *cptr = (char *)ptr;
    // Overwrite past the end of requested size
    cptr[50] = 'X';
    cptr[51] = 'Y';
    cptr[52] = 'Z';
    free(ptr); // Should detect corruption

    stats = get_allocator_stats();
    assert(stats.corruption_events == start_corruption + 1);

    // Double free
    ptr = malloc(20);
    free(ptr);
    free(ptr); // Should detect double free

    stats = get_allocator_stats();
    assert(stats.corruption_events == start_corruption + 2);

    printf("Canary tests passed!\n");
    return 0;
}
