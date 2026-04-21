#include "pseudo_malloc.h"
#include <stdio.h>


// test base dello stato interno dell'allocatore
int main(void) {
    int result;

    //all'inizio l'allocatore non deve essere inizializzato
    if (pseudo_malloc_is_initialized() != 0){
        printf("Test failed: allocator should start uninitialized\n");
        return 1;
    }

    if (pseudo_malloc_arena_size() != 0){
        printf("Test failed: initial arena size should be 0\n");
        return 1;
    }

    if (pseudo_malloc_has_memory() != 0) {
        printf("Test failed: allocator should start without memory\n");
        return 1;
    }

    //size 0 non e' valido: init deve fallire
    result = pseudo_malloc_init(0);
    if(result == 0){
        printf("Test failed: init with size 0 should fail\n");
        return 1;
    }

     //arena troppo piccola per contenere metadata
    result = pseudo_malloc_init(1);
    if(result == 0){
        printf("Test failed: init with tiny arena should fail\n");
        return 1;
    }


    // Init valida
    result = pseudo_malloc_init(1024);
    if (result != 0) {
        printf("Test failed: valid init should succeed\n");
        return 1;
    }

    if (pseudo_malloc_is_initialized() != 1){
        printf("Test failed: allocator should be initialized\n");
        return 1;
    }

    if (pseudo_malloc_arena_size() != 1024){
        printf("Test failed: arena size should be 1024/n");
        return 1;
    }

    if (pseudo_malloc_has_memory() != 1) {
        printf("Test failed: arena memory should be allocated\n");
        return 1;
    }


    //Doppia init: deve fallire
    result = pseudo_malloc_init(2048);
    if (result == 0) {
        printf("Test failed: double init should fail\n");
        return 1;
    }

    //Destroy valido
    pseudo_malloc_destroy();

    if (pseudo_malloc_is_initialized() != 0){
        printf("Test failed: allocator should be uninitialized after destroy\n");
        return 1;
    }

    if (pseudo_malloc_arena_size() != 0) {
        printf("Test failed: arena size should reset to 0 after destroy\n");
        return 1;
    }

    if (pseudo_malloc_has_memory() != 0) {
        printf("Test failed: arena memory should be released after destroy\n");
        return 1;
    }

    //secondo destroy: non deve rompere il programma
    pseudo_malloc_destroy();

    printf("All tests passed\n");
    return 0;

   
}