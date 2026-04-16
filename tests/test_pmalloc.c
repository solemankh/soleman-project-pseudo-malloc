#include "pseudo_malloc.h"

#include <stdio.h>

// test base dell'allocatore
int main(void) {

    // Inizializzazione con dimensione di test
    int result = pseudo_malloc_init(1024);
    
    // verifica esito init
    if (result != 0) {
        printf("Initialization failed\n");
        return 1;
    }

    printf("Initialization succeeded\n");
    
    // chiusura allocatore
    pseudo_malloc_destroy();

    return 0;
}