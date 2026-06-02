#include "wrouter.h"
#include "terminal.h"
#include <stdint.h>

wrouter_route_t *terminal_lookup(const terminals_t *terminals, uint16_t ref)
{
    // TODO custom binary search.
    for (uint16_t i = 0; i < terminals->count; i++)
        if (terminals->refs[i] == ref)
            return &terminals->base[i];

    return NULL;
}
