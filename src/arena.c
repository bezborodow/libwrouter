#include <stdlib.h>
#include <stddef.h>

void *arena_alloc(arena_t *a, size_t len)
{
    if (a->used + len > a->size) {
        size_t new_size = a->size ? a->size * 2 : 1024;

        while (new_size < a->used + len)
            new_size *= 2;

        void *new_ptr = realloc(a->base, new_size);
        if (!new_ptr)
            return NULL;

        a->base = new_ptr;
        a->size = new_size;
    }

    void *out = a->base + a->used;
    a->used += len;
    return out;
}

void arena_free(arena_t *a)
{
    free(a->base);
    a->size = 0;
    a->cursor = 0;
}
