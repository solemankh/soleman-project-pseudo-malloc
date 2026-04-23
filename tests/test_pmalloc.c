#include "pseudo_malloc.h"
#include <stdio.h>

// Test base della nuova allocazione buddy con backend mmap
int main(void) {
    int result;
    void* p1;
    void* p2;
    void* p3;

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
    result = pseudo_malloc_init(4096, 0);
    if (result == 0) {
        printf("Test failed: init with invalid levels should fail\n");
        return 1;
    }

    // arena non multipla della page size
    result = pseudo_malloc_init(5000, 4);
    if (result == 0) {
        printf("Test failed: init with non page-aligned arena should fail\n");
        return 1;
    }

    // init valida
    result = pseudo_malloc_init(4096, 4);
    if (result != 0) {
        printf("Test failed: valid init should succeed\n");
        return 1;
    }

    if (pseudo_malloc_is_initialized() != 1) {
        printf("Test failed: allocator should be initialized\n");
        return 1;
    }

    if (pseudo_malloc_arena_size() != 4096) {
        printf("Test failed: arena size should be 4096\n");
        return 1;
    }

    if (pseudo_malloc_num_levels() != 4) {
        printf("Test failed: number of levels should be 4\n");
        return 1;
    }

    // prima allocazione
    p1 = pseudo_malloc(100);
    if (p1 == NULL) {
        printf("Test failed: first allocation should succeed\n");
        return 1;
    }

    // seconda allocazione
    p2 = pseudo_malloc(100);
    if (p2 == NULL) {
        printf("Test failed: second allocation should succeed\n");
        return 1;
    }

    if (p1 == p2) {
        printf("Test failed: allocations should return different addresses\n");
        return 1;
    }

    // libero il primo e rialloco
    pseudo_free(p1);

    p3 = pseudo_malloc(100);
    if (p3 == NULL) {
        printf("Test failed: allocation after free should succeed\n");
        return 1;
    }

    // richiesta troppo grande
    if (pseudo_malloc(50000) != NULL) {
        printf("Test failed: oversized allocation should fail\n");
        return 1;
    }

    // libero tutto
    pseudo_free(p2);
    pseudo_free(p3);

    // doppia init non valida
    result = pseudo_malloc_init(4096, 5);
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