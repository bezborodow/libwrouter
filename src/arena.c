#include "arena.h"
#include <stdlib.h>
#include <stddef.h>

static arena_block_t *arena_new_block(size_t min_size)
{
    size_t size = min_size > ARENA_BLOCK_SIZE ? min_size : ARENA_BLOCK_SIZE;

    arena_block_t *b = malloc(sizeof(*b));
    if (!b) return NULL;

    b->mem = malloc(size);
    if (!b->mem) {
        free(b);
        return NULL;
    }

    b->used = 0;
    b->next = NULL;
    return b;
}

void *arena_alloc(arena_t *a, size_t len)
{
    if (!a->head) {
        a->head = arena_new_block(len);
        if (!a->head) return NULL;
    }

    arena_block_t *b = a->head;

    while (1) {
        if (b->used + len <= ARENA_BLOCK_SIZE) {
            void *out = b->mem + b->used;
            b->used += len;
            return out;
        }

        if (!b->next) {
            b->next = arena_new_block(len);
            if (!b->next) return NULL;
        }

        b = b->next;
    }
}

void arena_free(arena_t *a)
{
    arena_block_t *b = a->head;

    while (b) {
        arena_block_t *next = b->next;
        free(b->mem);
        free(b);
        b = next;
    }

    a->head = NULL;
}
