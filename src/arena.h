#pragma once
#include <stddef.h>

#define ARENA_BLOCK_SIZE 4096

typedef struct arena_block {
    unsigned char *base;
    size_t used;
    size_t size;
    struct arena_block *next;
} arena_block_t;

typedef struct {
    arena_block_t *head;
    arena_block_t *tail;
} arena_t;

void *arena_alloc(arena_t *a, size_t length);
void *arena_alloc_aligned(arena_t *a, size_t size, size_t align);
size_t arena_used(const arena_t *a);
void arena_free(arena_t *a);
