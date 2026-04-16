#include "pseudo_malloc.h"

#include <stdio.h>

// Implementazione iniziale (stub) : nessuna gestione reale della memoria 
int pseudo_malloc_init(size_t size) {

    // controllo base su parametro
    if (size == 0) {
        printf("Error: size must be > 0\n");
        return -1;
    }
    
    // bebug: verifica chiamata e parametro
    printf("pseudo_malloc_init called with size = %zu\n", size);
    return 0;
}

// stub: in futuro liberera' l'arena
void pseudo_malloc_destroy(void) {
    printf("pseudo_malloc_destroy called\n");
}