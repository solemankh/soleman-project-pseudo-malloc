# Pseudo Malloc con Buddy Allocator

Progetto universitario: implementazione di una pseudo-`malloc` in C basata su `mmap` e su un buddy allocator non a bitmap.

L'obiettivo è gestire manualmente un'area di memoria ottenuta dal sistema operativo, dividendo e riunendo blocchi secondo la logica del buddy allocator.

---

## Obiettivo del progetto

Il progetto implementa una piccola libreria che fornisce funzioni simili a `malloc` e `free`:

```c
void* pseudo_malloc(size_t size);
void pseudo_free(void* ptr);