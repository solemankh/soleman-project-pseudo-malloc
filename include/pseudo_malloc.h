#ifndef PSEUDO_MALLOC_H
#define PSEUDO_MALLOC_H

#include <stddef.h> //size_t per dimensioni in byte
 
 // Inizializza il buddy allocator 
int pseudo_malloc_init(size_t arena_size, int num_levels);

// Rilascio le risore dell'allocatore
void pseudo_malloc_destroy(void);

// Alloca memoria dall'arena gestita 
void* pseudo_malloc(size_t size);

// Libera memoria precedemtemente allocata
void pseudo_free(void* ptr);

// Funzioni di supporto per i test
int pseudo_malloc_is_initialized(void);
size_t pseudo_malloc_arena_size(void);
int pseudo_malloc_num_levels(void);



#endif