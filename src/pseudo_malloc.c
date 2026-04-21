#include "pseudo_malloc.h"
#include <stdio.h>
#include <stdlib.h>

// Struttura che rappresenta un blocco di memoria
typedef struct block_header {
    size_t size;          // dimensione del blocco
    int is_free;          // 1 = libero, 0 = occupato
    struct block_header* next; // puntatore al prossimo blocco
} block_header;

// Stato interno dell'allocatore
static int g_initialized = 0;
static size_t g_arena_size = 0;
// Puntatore base dell'arena
static void* g_memory = NULL; 

//Primo blocco dell'arena
static block_header* g_first_block = NULL;

// Inizializza l'allocatore e crea una prima arena lineare 
int pseudo_malloc_init(size_t size) {
    block_header* first_block;

    // La dimensione dell'arena deve essere valida 
    if (size == 0) {
        printf("Error: size must be > 0\n");
        return -1;
    }

    // evita doppia inizializzazione 
    if (g_initialized) {
        printf("Error: allocator already initialized\n");
        return -1;
    }

    // L'arena deve contenere almeno l'header di un blocco
    if (size <= sizeof(block_header)) {
        printf("Error: size too small for allocator metadata\n");
        return -1;
    }

    // Backend temporaneo: arena ottenuta con malloc
    g_memory = malloc(size);
    if (g_memory == NULL) {
        printf("Error: arena allocatoin failed\n");
        return -1;
    }

    g_initialized = 1;
    g_arena_size = size;
    
    // il primo blocco coincide con ;'inizio dell'arena 
    first_block = (block_header*) g_memory;
    first_block->size = size - sizeof(block_header);
    first_block->is_free = 1;
    first_block->next = NULL;

    g_first_block = first_block;


    // Debug temporaneo: conferma init corretta
    printf("pseudo_malloc_init: arena size = %lu\n", (unsigned long ) g_arena_size);
    printf("pseudo_malloc_init: first block size = %lu\n", (unsigned long ) g_first_block->size);
    return 0;
}

// distrugge l'allocatore e libera l'arena
void pseudo_malloc_destroy(void) {
    // se non era inizializzato, non c'è nulla da distruggere
    if(!g_initialized){
        printf("Warning: destroy called before init\n"); 
        return;
    }

    free(g_memory);

    g_memory = NULL;
    g_first_block = NULL;
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

// verifica se l'arena e' stata allocata
int pseudo_malloc_has_memory(void) {
    return (g_memory != NULL);
}

