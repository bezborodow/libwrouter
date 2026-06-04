#include "router.h"
#include "wrouter.h"
#include "terminal.h"
#include <stdint.h>
#include <stdlib.h>

void terminal_append(terminals_t *terminals, uint16_t ref, const wrouter_route_t route)
{
    terminals->refs[terminals->count] = ref;
    terminals->base[terminals->count++] = route;
}

wrouter_route_t *terminal_lookup(const terminals_t *terminals, uint16_t ref)
{
    // TODO custom binary search.
    for (uint16_t i = 0; i < terminals->count; i++)
        if (terminals->refs[i] == ref)
            return &terminals->base[i];

    return NULL;
}

void terminals_free(terminals_t *terminals)
{
    if (terminals == NULL)
        return;

    free(terminals->base);
    free(terminals->refs);
}
