#include "pseudo_malloc.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#define MAX_LEVELS 16

// Metadato di un blocco buddy
typedef struct BuddyBlock {
    int level;                   // livello del blocco
    int is_free;                 // 1 se libero, 0 se occupato
    char* start;                 // inizio del blocco nell'arena
    size_t size;                 // dimensione del blocco
    struct BuddyBlock* next;     // prossimo blocco nella free list
    struct BuddyBlock* buddy;    // buddy logico
    struct BuddyBlock* parent;   // blocco padre
    struct BuddyBlock* left;     // figlio sinistro
    struct BuddyBlock* right;    // figlio destro
} BuddyBlock;

// Stato globale del buddy allocator
typedef struct BuddyAllocator {
    int initialized;                     // stato init
    int num_levels;                      // profondita' del buddy
    size_t arena_size;                   // dimensione totale arena
    size_t min_bucket_size;              // blocco minimo
    char* memory;                        // arena gestita
    BuddyBlock* free_lists[MAX_LEVELS];  // una free list per livello
    BuddyBlock* root;                    // blocco iniziale
} BuddyAllocator;

static BuddyAllocator g_allocator = {0};

// Calcola la dimensione minima dei blocchi
static size_t compute_min_bucket_size(size_t arena_size, int num_levels) {
    return arena_size >> num_levels;
}

// Ritorna la dimensione di un blocco a un certo livello
static size_t block_size_for_level(int level) {
    return g_allocator.arena_size >> level;
}

// Inserisce un blocco in testa alla free list del livello
static void push_free_block(BuddyBlock* block) {
    block->next = g_allocator.free_lists[block->level];
    g_allocator.free_lists[block->level] = block;
    block->is_free = 1;
}

// Rimuove un blocco specifico dalla free list del suo livello
static void remove_free_block(BuddyBlock* block) {
    BuddyBlock* current;
    BuddyBlock* previous;
    int level;

    if (block == NULL) {
        return;
    }

    level = block->level;
    current = g_allocator.free_lists[level];
    previous = NULL;

    while (current != NULL) {
        if (current == block) {
            if (previous == NULL) {
                g_allocator.free_lists[level] = current->next;
            } else {
                previous->next = current->next;
            }

            current->next = NULL;
            return;
        }

        previous = current;
        current = current->next;
    }
}

// Estrae il primo blocco libero da un livello
static BuddyBlock* pop_free_block(int level) {
    BuddyBlock* block = g_allocator.free_lists[level];

    if (block == NULL) {
        return NULL;
    }

    g_allocator.free_lists[level] = block->next;
    block->next = NULL;
    return block;
}

// Cerca il livello piu' piccolo che puo' contenere la richiesta
static int level_for_request(size_t request_size) {
    int level;

    for (level = g_allocator.num_levels; level >= 0; --level) {
        if (block_size_for_level(level) >= request_size) {
            if (level == g_allocator.num_levels ||
                block_size_for_level(level + 1) < request_size) {
                return level;
            }
        }
    }

    return -1;
}

// Divide un blocco in due buddy al livello successivo
static BuddyBlock* split_block(BuddyBlock* parent) {
    BuddyBlock* left;
    BuddyBlock* right;
    size_t child_size;

    if (parent == NULL) {
        return NULL;
    }

    if (parent->level >= g_allocator.num_levels) {
        return NULL;
    }

    child_size = parent->size / 2;

    left = (BuddyBlock*) malloc(sizeof(BuddyBlock));
    right = (BuddyBlock*) malloc(sizeof(BuddyBlock));

    if (left == NULL || right == NULL) {
        free(left);
        free(right);
        return NULL;
    }

    left->level = parent->level + 1;
    left->is_free = 1;
    left->start = parent->start;
    left->size = child_size;
    left->next = NULL;
    left->buddy = right;
    left->parent = parent;
    left->left = NULL;
    left->right = NULL;

    right->level = parent->level + 1;
    right->is_free = 1;
    right->start = parent->start + child_size;
    right->size = child_size;
    right->next = NULL;
    right->buddy = left;
    right->parent = parent;
    right->left = NULL;
    right->right = NULL;

    parent->left = left;
    parent->right = right;
    parent->is_free = 0;

    // Il buddy destro torna nella free list
    push_free_block(right);

    // Il buddy sinistro prosegue nella discesa
    return left;
}

// Ottiene un blocco libero del livello richiesto, facendo split se serve
static BuddyBlock* obtain_block_at_level(int target_level) {
    int level;
    BuddyBlock* block;

    for (level = target_level; level >= 0; --level) {
        if (g_allocator.free_lists[level] != NULL) {
            block = pop_free_block(level);

            while (block != NULL && block->level < target_level) {
                block = split_block(block);
            }

            return block;
        }
    }

    return NULL;
}

// Prova a fare merge ricorsivo verso il padre
static BuddyBlock* try_merge(BuddyBlock* block) {
    BuddyBlock* buddy;
    BuddyBlock* parent;

    if (block == NULL) {
        return NULL;
    }

    while (block->parent != NULL) {
        buddy = block->buddy;
        parent = block->parent;

        if (buddy == NULL) {
            break;
        }

        if (!buddy->is_free) {
            break;
        }

        if (buddy->left != NULL || buddy->right != NULL) {
            break;
        }

        if (block->left != NULL || block->right != NULL) {
            break;
        }

        remove_free_block(block);
        remove_free_block(buddy);

        free(parent->left);
        free(parent->right);

        parent->left = NULL;
        parent->right = NULL;
        parent->is_free = 1;
        parent->next = NULL;

        block = parent;
    }

    return block;
}

// Libera ricorsivamente l'albero dei metadati
static void destroy_block_tree(BuddyBlock* block) {
    if (block == NULL) {
        return;
    }

    destroy_block_tree(block->left);
    destroy_block_tree(block->right);
    free(block);
}

// Inizializza la struttura del buddy allocator usando mmap per l'arena
int pseudo_malloc_init(size_t arena_size, int num_levels) {
    int i;
    BuddyBlock* root;
    long page_size;
    void* mapped_memory;

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

    page_size = sysconf(_SC_PAGESIZE);
    if (page_size <= 0) {
        printf("Error: could not read system page size\n");
        return -1;
    }

    if (arena_size % (size_t) page_size != 0) {
        printf("Error: arena_size must be a multiple of page size\n");
        return -1;
    }

    mapped_memory = mmap(
        NULL,
        arena_size,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1,
        0
    );

    if (mapped_memory == MAP_FAILED) {
        perror("mmap failed");
        return -1;
    }

    root = (BuddyBlock*) malloc(sizeof(BuddyBlock));
    if (root == NULL) {
        munmap(mapped_memory, arena_size);
        printf("Error: root metadata allocation failed\n");
        return -1;
    }

    g_allocator.initialized = 1;
    g_allocator.num_levels = num_levels;
    g_allocator.arena_size = arena_size;
    g_allocator.min_bucket_size = compute_min_bucket_size(arena_size, num_levels);
    g_allocator.memory = (char*) mapped_memory;

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
    root->left = NULL;
    root->right = NULL;

    g_allocator.root = root;
    g_allocator.free_lists[0] = root;

    printf("pseudo_malloc_init: arena size = %lu\n", (unsigned long) g_allocator.arena_size);
    printf("pseudo_malloc_init: num levels = %d\n", g_allocator.num_levels);
    printf("pseudo_malloc_init: min bucket size = %lu\n", (unsigned long) g_allocator.min_bucket_size);
    printf("pseudo_malloc_init: page size = %ld\n", page_size);

    return 0;
}

// Alloca memoria cercando il livello adatto nel buddy system
void* pseudo_malloc(size_t size) {
    BuddyBlock* block;
    size_t request_size;
    int target_level;

    if (size == 0) {
        return NULL;
    }

    if (!g_allocator.initialized) {
        return NULL;
    }

    // Salviamo il puntatore al metadato nei primi byte del blocco
    request_size = size + sizeof(BuddyBlock*);

    if (request_size > g_allocator.arena_size) {
        return NULL;
    }

    target_level = level_for_request(request_size);
    if (target_level < 0) {
        return NULL;
    }

    block = obtain_block_at_level(target_level);
    if (block == NULL) {
        return NULL;
    }

    block->is_free = 0;
    *((BuddyBlock**) block->start) = block;

    // Restituiamo l'indirizzo dopo il metadato utente
    return block->start + sizeof(BuddyBlock*);
}

// Libera un blocco e prova a fare merge con il buddy
void pseudo_free(void* ptr) {
    BuddyBlock* block;

    if (ptr == NULL) {
        return;
    }

    if (!g_allocator.initialized) {
        return;
    }

    block = *((BuddyBlock**) ((char*) ptr - sizeof(BuddyBlock*)));
    if (block == NULL) {
        return;
    }

    block->is_free = 1;
    block->next = NULL;

    push_free_block(block);
    block = try_merge(block);

    remove_free_block(block);
    push_free_block(block);
}

void pseudo_malloc_dump(void) {
    int i;
    BuddyBlock* current;

    if (!g_allocator.initialized) {
        printf("Allocator not initialized\n");
        return;
    }

    printf("\n===== BUDDY ALLOCATOR DUMP =====\n");

    for (i = 0; i <= g_allocator.num_levels; i++) {
        printf("Level %d (size %lu): ", i, (unsigned long) block_size_for_level(i));
        current = g_allocator.free_lists[i];

        if (current == NULL) {
            printf("empty");
        }

        while (current != NULL) {
            printf("[free] -> ");
            current = current->next;
        }

        printf("NULL\n");
    }

    printf("================================\n\n");

    
}

// Rilascia le risorse dell'allocatore
void pseudo_malloc_destroy(void) {
    if (!g_allocator.initialized) {
        printf("Warning: destroy called before init\n");
        return;
    }

    destroy_block_tree(g_allocator.root);
    munmap(g_allocator.memory, g_allocator.arena_size);

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