#include "pseudo_malloc.h"
#include <stdio.h>

#define SMALL_ALLOCS 8

// Test finali del buddy allocator con mmap, split, free, merge e casi limite
int main(void) {
    int result;
    int i;

    void* p1;
    void* p2;
    void* p3;
    void* small_blocks[SMALL_ALLOCS];

    // Stato iniziale
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

    // malloc prima della init: deve fallire
    if (pseudo_malloc(32) != NULL) {
        printf("Test failed: allocation before init should fail\n");
        return 1;
    }

    // free(NULL) deve essere sicura
    pseudo_free(NULL);

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

    printf("\n--- Initial state ---\n");
    pseudo_malloc_dump();

    // malloc(0) deve fallire
    if (pseudo_malloc(0) != NULL) {
        printf("Test failed: allocation with size 0 should fail\n");
        return 1;
    }

    // Prima allocazione
    p1 = pseudo_malloc(100);
    if (p1 == NULL) {
        printf("Test failed: first allocation should succeed\n");
        return 1;
    }

    // Seconda allocazione
    p2 = pseudo_malloc(100);
    if (p2 == NULL) {
        printf("Test failed: second allocation should succeed\n");
        return 1;
    }

    if (p1 == p2) {
        printf("Test failed: allocations should return different addresses\n");
        return 1;
    }

    printf("\n--- After two allocations ---\n");
    pseudo_malloc_dump();

    // Libero il primo e rialloco
    pseudo_free(p1);

    printf("\n--- After freeing first allocation ---\n");
    pseudo_malloc_dump();

    p3 = pseudo_malloc(100);
    if (p3 == NULL) {
        printf("Test failed: allocation after free should succeed\n");
        return 1;
    }

    printf("\n--- After allocation following free ---\n");
    pseudo_malloc_dump();

    // Richiesta troppo grande: deve fallire
    if (pseudo_malloc(50000) != NULL) {
        printf("Test failed: oversized allocation should fail\n");
        return 1;
    }

    // Libero i blocchi rimasti in ordine diverso
    pseudo_free(p3);
    pseudo_free(p2);

    printf("\n--- After freeing main allocations ---\n");
    pseudo_malloc_dump();

    // Stress test semplice: tante allocazioni piccole
    for (i = 0; i < SMALL_ALLOCS; i++) {
        small_blocks[i] = pseudo_malloc(32);
        if (small_blocks[i] == NULL) {
            printf("Test failed: small allocation %d should succeed\n", i);
            return 1;
        }
    }

    printf("\n--- After small allocations ---\n");
    pseudo_malloc_dump();

    // Libero prima gli indici pari
    for (i = 0; i < SMALL_ALLOCS; i += 2) {
        pseudo_free(small_blocks[i]);
    }

    printf("\n--- After freeing even small blocks ---\n");
    pseudo_malloc_dump();

    // Poi gli indici dispari
    for (i = 1; i < SMALL_ALLOCS; i += 2) {
        pseudo_free(small_blocks[i]);
    }

    printf("\n--- After freeing all small blocks ---\n");
    pseudo_malloc_dump();

    // Dopo aver liberato tutto, una grossa allocazione deve riuscire
    p1 = pseudo_malloc(2000);
    if (p1 == NULL) {
        printf("Test failed: large allocation after merge should succeed\n");
        return 1;
    }

    printf("\n--- After large allocation following full merge ---\n");
    pseudo_malloc_dump();

    pseudo_free(p1);

    printf("\n--- Final state before destroy ---\n");
    pseudo_malloc_dump();

    // Doppia init non valida
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

    // destroy ripetuto: non deve rompere il programma
    pseudo_malloc_destroy();

    printf("All final tests passed\n");
    return 0;
}