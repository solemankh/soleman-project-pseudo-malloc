#include "pseudo_malloc.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define MAX_LEVELS 20

// Buddy allocator rappresentato solo tramite bitmap e indici del tree
typedef struct BitmapBuddyAllocator {
    int initialized;
    int num_levels;
    size_t arena_size;
    size_t min_bucket_size;

    char* memory;

    int max_node_idx;
    size_t bitmap_bytes;

    // split_bitmap[idx] = 1 se il nodo e' stato diviso
    unsigned char* split_bitmap;

    // used_bitmap[idx] = 1 se il nodo e' occupato
    unsigned char* used_bitmap;
} BitmapBuddyAllocator;

static BitmapBuddyAllocator g_allocator = {0};

// -----------------------------
// Bit helpers
// -----------------------------

static int bit_get(unsigned char* bitmap, int idx) {
    return (bitmap[idx / 8] >> (idx % 8)) & 1;
}

static void bit_set(unsigned char* bitmap, int idx) {
    bitmap[idx / 8] |= (unsigned char)(1u << (idx % 8));
}

static void bit_clear(unsigned char* bitmap, int idx) {
    bitmap[idx / 8] &= (unsigned char)~(1u << (idx % 8));
}

// -----------------------------
// Indexed buddy tree helpers
// -----------------------------

static int first_idx(int level) {
    return 1 << level;
}

static int level_idx(int idx) {
    int level = 0;

    while (idx > 1) {
        idx = idx / 2;
        level++;
    }

    return level;
}

static int parent_idx(int idx) {
    return idx / 2;
}

static int buddy_idx(int idx) {
    if (idx & 1) {
        return idx - 1;
    }

    return idx + 1;
}

static int left_child_idx(int idx) {
    return idx * 2;
}

static int right_child_idx(int idx) {
    return idx * 2 + 1;
}

static size_t block_size_for_level(int level) {
    return g_allocator.arena_size >> level;
}

static char* node_start_address(int idx) {
    int level = level_idx(idx);
    int offset = idx - first_idx(level);
    size_t block_size = block_size_for_level(level);

    return g_allocator.memory + ((size_t)offset * block_size);
}

static int is_node_free(int idx) {
    return !bit_get(g_allocator.split_bitmap, idx) &&
           !bit_get(g_allocator.used_bitmap, idx);
}

// -----------------------------
// Buddy logic
// -----------------------------

static int level_for_request(size_t request_size) {
    int level;

    for (level = g_allocator.num_levels; level >= 0; --level) {
        if (block_size_for_level(level) >= request_size) {
            return level;
        }
    }

    return -1;
}

// Cerca ricorsivamente un nodo libero al livello richiesto.
// Se serve, divide nodi piu' grandi marcandoli come split.
static int allocate_node(int idx, int current_level, int target_level) {
    int left;
    int right;
    int result;

    if (idx > g_allocator.max_node_idx) {
        return 0;
    }

    if (bit_get(g_allocator.used_bitmap, idx)) {
        return 0;
    }

    if (current_level == target_level) {
        if (is_node_free(idx)) {
            bit_set(g_allocator.used_bitmap, idx);
            return idx;
        }

        return 0;
    }

    if (is_node_free(idx)) {
        bit_set(g_allocator.split_bitmap, idx);
    }

    if (!bit_get(g_allocator.split_bitmap, idx)) {
        return 0;
    }

    left = left_child_idx(idx);
    right = right_child_idx(idx);

    result = allocate_node(left, current_level + 1, target_level);
    if (result != 0) {
        return result;
    }

    return allocate_node(right, current_level + 1, target_level);
}

// Dopo una free, prova a risalire unendo buddy liberi.
static void try_merge_up(int idx) {
    int parent;
    int buddy;

    while (idx > 1) {
        parent = parent_idx(idx);
        buddy = buddy_idx(idx);

        if (!is_node_free(idx) || !is_node_free(buddy)) {
            break;
        }

        bit_clear(g_allocator.split_bitmap, parent);
        idx = parent;
    }
}

// -----------------------------
// Public API
// -----------------------------

int pseudo_malloc_init(size_t arena_size, int num_levels) {
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

    if (arena_size % (size_t)page_size != 0) {
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

    g_allocator.max_node_idx = (1 << (num_levels + 1)) - 1;
    g_allocator.bitmap_bytes = ((size_t)g_allocator.max_node_idx + 8u) / 8u;

    g_allocator.split_bitmap = (unsigned char*)calloc(g_allocator.bitmap_bytes, 1);
    g_allocator.used_bitmap = (unsigned char*)calloc(g_allocator.bitmap_bytes, 1);

    if (g_allocator.split_bitmap == NULL || g_allocator.used_bitmap == NULL) {
        free(g_allocator.split_bitmap);
        free(g_allocator.used_bitmap);
        munmap(mapped_memory, arena_size);
        printf("Error: bitmap allocation failed\n");
        return -1;
    }

    g_allocator.initialized = 1;
    g_allocator.num_levels = num_levels;
    g_allocator.arena_size = arena_size;
    g_allocator.min_bucket_size = arena_size >> num_levels;
    g_allocator.memory = (char*)mapped_memory;

    printf("pseudo_malloc_init: arena size = %lu\n", (unsigned long)arena_size);
    printf("pseudo_malloc_init: num levels = %d\n", num_levels);
    printf("pseudo_malloc_init: min bucket size = %lu\n",
           (unsigned long)g_allocator.min_bucket_size);
    printf("pseudo_malloc_init: bitmap nodes = %d\n", g_allocator.max_node_idx);

    return 0;
}

void* pseudo_malloc(size_t size) {
    size_t request_size;
    int target_level;
    int node_idx;
    char* start;

    if (size == 0) {
        return NULL;
    }

    if (!g_allocator.initialized) {
        return NULL;
    }

    // Nel record di controllo salvo l'indice del nodo, non un puntatore.
    request_size = size + sizeof(int);

    if (request_size > g_allocator.arena_size) {
        return NULL;
    }

    target_level = level_for_request(request_size);
    if (target_level < 0) {
        return NULL;
    }

    node_idx = allocate_node(1, 0, target_level);
    if (node_idx == 0) {
        return NULL;
    }

    start = node_start_address(node_idx);

    *((int*)start) = node_idx;

    return start + sizeof(int);
}

void pseudo_free(void* ptr) {
    char* control_record;
    int node_idx;

    if (ptr == NULL) {
        return;
    }

    if (!g_allocator.initialized) {
        return;
    }

    control_record = (char*)ptr - sizeof(int);
    node_idx = *((int*)control_record);

    if (node_idx <= 0 || node_idx > g_allocator.max_node_idx) {
        return;
    }

    if (!bit_get(g_allocator.used_bitmap, node_idx)) {
        return;
    }

    bit_clear(g_allocator.used_bitmap, node_idx);
    try_merge_up(node_idx);
}

void pseudo_malloc_dump(void) {
    int level;
    int idx;
    int first;
    int last;

    if (!g_allocator.initialized) {
        printf("Allocator not initialized\n");
        return;
    }

    printf("\n===== BITMAP BUDDY DUMP =====\n");

    for (level = 0; level <= g_allocator.num_levels; ++level) {
        first = first_idx(level);
        last = (1 << (level + 1)) - 1;

        printf("Level %d (size %lu): ",
               level,
               (unsigned long)block_size_for_level(level));

        for (idx = first; idx <= last; ++idx) {
            if (bit_get(g_allocator.used_bitmap, idx)) {
                printf("U");
            } else if (bit_get(g_allocator.split_bitmap, idx)) {
                printf("S");
            } else {
                printf("F");
            }
        }

        printf("\n");
    }

    printf("Legend: F=free, S=split, U=used\n");
    printf("=============================\n\n");
}

void pseudo_malloc_destroy(void) {
    if (!g_allocator.initialized) {
        printf("Warning: destroy called before init\n");
        return;
    }

    munmap(g_allocator.memory, g_allocator.arena_size);

    free(g_allocator.split_bitmap);
    free(g_allocator.used_bitmap);

    memset(&g_allocator, 0, sizeof(g_allocator));

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