#include "arena.h"
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>

static arena_block_t *arena_new_block(size_t min_size)
{
    size_t size = min_size > ARENA_BLOCK_SIZE ? min_size : ARENA_BLOCK_SIZE;

    arena_block_t *b = calloc(1, sizeof(*b));
    if (!b)
        return NULL;

    b->base = malloc(size);
    if (!b->base) {
        free(b);
        return NULL;
    }

    b->size = size;

    return b;
}

void *arena_alloc_aligned(arena_t *a, size_t size, size_t align)
{
    if (!a->head) {
        a->head = a->tail = arena_new_block(size + align - 1);
        if (!a->head)
            return NULL;
    }

    for (arena_block_t *b = a->tail;; b = b->next) {

        uintptr_t base = (uintptr_t)b->base + b->used;
        uintptr_t aligned = (base + (align - 1)) & ~(align - 1);

        size_t padding = aligned - base;
        size_t total = padding + size;

        if (b->used + total <= b->size) {
            b->used += total;
            return (void *)aligned;
        }

        if (!b->next) {
            b->next = arena_new_block(size + align - 1);
            if (!b->next)
                return NULL;

            a->tail = b->next;
        }
    }
}

void *arena_alloc(arena_t *a, size_t length)
{
    return arena_alloc_aligned(a, length, _Alignof(char));
}

void arena_free(arena_t *a)
{
    if (a == NULL)
        return;

    arena_block_t *b = a->head;

    while (b) {
        arena_block_t *next = b->next;
        free(b->base);
        free(b);
        b = next;
    }

    a->head = NULL;
}

size_t arena_used(const arena_t *a)
{
    size_t used = 0;

    arena_block_t *b = a->head;

    while (b) {
        arena_block_t *next = b->next;
        used += b->used;
        b = next;
    }

    return used;
}
