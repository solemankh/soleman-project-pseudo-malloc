#ifndef PSEUDO_MALLOC_H
#define PSEUDO_MALLOC_H

#include <stddef.h> //size_t per dimensioni in byte
 
 // inizializza l'allocatore con una certa dimensione di arena 
int pseudo_malloc_init(size_t size);

// distrugge l'allocatore e libera la risorse
void pseudo_malloc_destroy(void);

#endif