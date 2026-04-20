#ifndef PSEUDO_MALLOC_H
#define PSEUDO_MALLOC_H

#include <stddef.h> //size_t per dimensioni in byte
 
 // inizializza l'allocatore con una certa dimensione di arena 
int pseudo_malloc_init(size_t size);

// distrugge l'allocatore e libera la risorse
void pseudo_malloc_destroy(void);

// ritorna 1 se l'allocatore e' inizializzato, 0 altrimenti
int pseudo_malloc_is_initialized(void);

// ritorna la dimansione corrente dell'arena, 0 se non inizializzato
size_t pseudo_malloc_arena_size(void);


#endif