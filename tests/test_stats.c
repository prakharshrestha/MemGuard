#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "allocator.h"

int main() {
    printf("Running stats tests...\n");

    allocator_stats initial_stats = get_allocator_stats();
    
    void *p1 = malloc(100);
    void *p2 = malloc(200);

    allocator_stats stats = get_allocator_stats();
    assert(stats.active_allocations == initial_stats.active_allocations + 2);
    
    free(p1);
    
    stats = get_allocator_stats();
    assert(stats.active_allocations == initial_stats.active_allocations + 1);
    
    free(p2);

    stats = get_allocator_stats();
    assert(stats.active_allocations == initial_stats.active_allocations);
    
    printf("Stats tests passed!\n");
    return 0;
}
