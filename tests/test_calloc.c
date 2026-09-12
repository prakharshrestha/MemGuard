#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include "allocator.h"

int main() {
    printf("Running calloc tests...\n");

    // Normal calloc
    int *arr = (int *)calloc(10, sizeof(int));
    assert(arr != NULL);
    for (int i = 0; i < 10; i++) {
        assert(arr[i] == 0);
    }
    free(arr);

    // calloc(0, size)
    void *ptr = calloc(0, 100);
    assert(ptr == NULL);

    // calloc(num, 0)
    ptr = calloc(100, 0);
    assert(ptr == NULL);

    // Integer overflow case
    ptr = calloc(SIZE_MAX, 2);
    assert(ptr == NULL);

    printf("Calloc tests passed!\n");
    return 0;
}
