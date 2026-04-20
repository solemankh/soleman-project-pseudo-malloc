#include "pseudo_malloc.h"
#include <stdio.h>

// Stato interno dell'allocatore
static int g_initialized = 0;
static size_t g_arena_size = 0;

// Implementazione iniziale (stub) : nessuna gestione reale della memoria 
int pseudo_malloc_init(size_t size) {

    // controllo base su parametro
    if (size == 0) {
        printf("Error: size must be > 0\n");
        return -1;
    }

    // evita doppia inizializzazione 
    if (g_initialized) {
        printf("Error: allocator already initialized\n");
        return -1;
    }

    g_initialized = 1;
    g_arena_size = size;
    
    // Debug temporaneo: conferma init corretta
    printf("pseudo_malloc_init: arena size = %lu\n", (unsigned long ) g_arena_size);
    return 0;
}

// distrugge l'allocatore e resetta lo stato
void pseudo_malloc_destroy(void) {
    // se non era inizializzato, non c'è nulla da distruggere
    if(!g_initialized){
        printf("Warning: destroy called before init\n"); 
        return;
    }

    g_initialized = 0;
    g_arena_size = 0;

    printf("pseudo_malloc_destroy called\n");
}

//espone lo stato di inizializzazione per i test
int pseudo_malloc_is_initialized(void){
    return g_initialized;
}

//espone la dimensione dell'arena per i test
size_t pseudo_malloc_arena_size(void){
    return g_arena_size;
}
