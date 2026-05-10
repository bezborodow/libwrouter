#include <stddef.h>

#ifndef WROUTER_ARENA_H
#define WROUTER_ARENA_H

#define ARENA_BLOCK_SIZE 4096

typedef struct arena_block {
    char *mem;
    size_t used;
    struct arena_block *next;
} arena_block_t;

typedef struct {
    arena_block_t *head;
} arena_t;

void *arena_alloc(arena_t *a, size_t len);
void arena_free(arena_t *a);

#endif
