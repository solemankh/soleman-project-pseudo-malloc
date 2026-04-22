#include "pseudo_malloc.h"
#include <stdio.h>
#include <stdlib.h>

#define MAX_LEVELS 16

// Metadato di un blocco buddy
typedef struct BuddyBlock {
    int level;                  // livello del blocco
    int is_free;                // 1 se libero, 0 se occupato
    char* start;                // inizio del blocco nell'arena
    size_t size;                // dimensione del blocco
    struct BuddyBlock* next;    // prossimo blocco nella free list
    struct BuddyBlock* buddy;   // buddy logico
    struct BuddyBlock* parent;  // blocco padre
} BuddyBlock;

// Stato globale del buddy allocator
typedef struct BuddyAllocator {
    int initialized;                    // stato init
    int num_levels;                     // numero di livelli
    size_t arena_size;                  // dimensione totale arena
    size_t min_bucket_size;             // blocco minimo
    char* memory;                       // arena gestita
    BuddyBlock* free_lists[MAX_LEVELS]; // una free list per livello
    BuddyBlock* root;                   // blocco iniziale
} BuddyAllocator;

static BuddyAllocator g_allocator = {0};

// Calcola la dimensione minima dei blocchi
static size_t compute_min_bucket_size(size_t arena_size, int num_levels) {
    return arena_size >> num_levels;
}

// Inizializza la struttura del buddy allocator
int pseudo_malloc_init(size_t arena_size, int num_levels) {
    int i;
    BuddyBlock* root;

    if (arena_size == 0) {
        printf("Error: arena_size must be > 0\n");
        return -1;
    }

    if (num_levels <= 0 || num_levels >= MAX_LEVELS) {
        printf("Error: invalid number of levels\n");
        return -1;
    }

    if (g_allocator.initialized) {
        printf("Error: allocator already initialized\n");
        return -1;
    }

    g_allocator.memory = (char*) malloc(arena_size);
    if (g_allocator.memory == NULL) {
        printf("Error: arena allocation failed\n");
        return -1;
    }

    root = (BuddyBlock*) malloc(sizeof(BuddyBlock));
    if (root == NULL) {
        free(g_allocator.memory);
        g_allocator.memory = NULL;
        printf("Error: root metadata allocation failed\n");
        return -1;
    }

    g_allocator.initialized = 1;
    g_allocator.num_levels = num_levels;
    g_allocator.arena_size = arena_size;
    g_allocator.min_bucket_size = compute_min_bucket_size(arena_size, num_levels);

    for (i = 0; i < MAX_LEVELS; i++) {
        g_allocator.free_lists[i] = NULL;
    }

    // Il blocco root rappresenta tutta l'arena
    root->level = 0;
    root->is_free = 1;
    root->start = g_allocator.memory;
    root->size = arena_size;
    root->next = NULL;
    root->buddy = NULL;
    root->parent = NULL;

    g_allocator.root = root;
    g_allocator.free_lists[0] = root;

    printf("pseudo_malloc_init: arena size = %lu\n", (unsigned long) g_allocator.arena_size);
    printf("pseudo_malloc_init: num levels = %d\n", g_allocator.num_levels);
    printf("pseudo_malloc_init: min bucket size = %lu\n", (unsigned long) g_allocator.min_bucket_size);

    return 0;
}

// Per ora allocazione non ancora implementata
void* pseudo_malloc(size_t size) {
    (void) size;
    return NULL;
}

// Per ora free non ancora implementato
void pseudo_free(void* ptr) {
    (void) ptr;
}

// Rilascia le risorse dell'allocatore
void pseudo_malloc_destroy(void) {
    if (!g_allocator.initialized) {
        printf("Warning: destroy called before init\n");
        return;
    }

    free(g_allocator.root);
    free(g_allocator.memory);

    g_allocator.root = NULL;
    g_allocator.memory = NULL;
    g_allocator.initialized = 0;
    g_allocator.num_levels = 0;
    g_allocator.arena_size = 0;
    g_allocator.min_bucket_size = 0;

    printf("pseudo_malloc_destroy called\n");
}

int pseudo_malloc_is_initialized(void) {
    return g_allocator.initialized;
}

size_t pseudo_malloc_arena_size(void) {
    return g_allocator.arena_size;
}

int pseudo_malloc_num_levels(void) {
    return g_allocator.num_levels;
}