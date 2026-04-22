#include "pseudo_malloc.h"
#include <stdio.h>

// Test base della nuova struttura buddy
int main(void) {
    int result;

    if (pseudo_malloc_is_initialized() != 0) {
        printf("Test failed: allocator should start uninitialized\n");
        return 1;
    }

    if (pseudo_malloc_arena_size() != 0) {
        printf("Test failed: initial arena size should be 0\n");
        return 1;
    }

    if (pseudo_malloc_num_levels() != 0) {
        printf("Test failed: initial number of levels should be 0\n");
        return 1;
    }

    // arena non valida
    result = pseudo_malloc_init(0, 4);
    if (result == 0) {
        printf("Test failed: init with arena_size 0 should fail\n");
        return 1;
    }

    // livelli non validi
    result = pseudo_malloc_init(1024, 0);
    if (result == 0) {
        printf("Test failed: init with invalid levels should fail\n");
        return 1;
    }

    // init valida
    result = pseudo_malloc_init(1024, 4);
    if (result != 0) {
        printf("Test failed: valid init should succeed\n");
        return 1;
    }

    if (pseudo_malloc_is_initialized() != 1) {
        printf("Test failed: allocator should be initialized\n");
        return 1;
    }

    if (pseudo_malloc_arena_size() != 1024) {
        printf("Test failed: arena size should be 1024\n");
        return 1;
    }

    if (pseudo_malloc_num_levels() != 4) {
        printf("Test failed: number of levels should be 4\n");
        return 1;
    }

    // doppia init non valida
    result = pseudo_malloc_init(2048, 5);
    if (result == 0) {
        printf("Test failed: double init should fail\n");
        return 1;
    }

    pseudo_malloc_destroy();

    if (pseudo_malloc_is_initialized() != 0) {
        printf("Test failed: allocator should be uninitialized after destroy\n");
        return 1;
    }

    printf("All tests passed\n");
    return 0;
}