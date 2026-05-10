#include <stddef.h>

#ifndef WROUTER_ARENA_H
#define WROUTER_ARENA_H

typedef struct {
    char *base;
    size_t used;
    size_t size;
} arena_t;

void *arena_alloc(arena_t *a, size_t len);

#endif
