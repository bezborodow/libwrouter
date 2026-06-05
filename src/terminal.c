#include "router.h"
#include "wrouter.h"
#include "terminal.h"
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>

int terminal_alloc(terminals_t *terminals, size_t n)
{
    // Allocate refs.
    terminals->refs = calloc(n, sizeof(uint16_t));
    if (terminals->refs == NULL)
        goto failure;

    // Allocate terminals.
    terminals->base = calloc(n, sizeof(wrouter_route_t));
    if (terminals->base == NULL)
        goto failure;

    return 0;

failure:
    terminals_free(terminals);
    return -ENOMEM;
}

void terminal_append(terminals_t *terminals, uint16_t ref, const wrouter_route_t route)
{
    terminals->refs[terminals->count] = ref;
    terminals->base[terminals->count++] = route;
}

wrouter_route_t *terminal_lookup(const terminals_t *terminals, uint16_t ref)
{
    if (terminals->count > 16) {

        // See binary search example from 6.4 Pointers to Structures, K&R C 2nd
        // ed. (ANSI), page 137.
        uint16_t mid, low = 0, high = terminals->count;

        int32_t cond;
        while (low < high) {
            mid = low + (high - low) / 2;
            if ((cond = ref - terminals->refs[mid]) < 0)
                high = mid;
            else if (cond > 0)
                low = mid + 1;
            else
                return &terminals->base[mid];
        }

    } else {

        // Linear search by ref.
        for (uint16_t i = 0; i < terminals->count; i++) {
            if (terminals->refs[i] == ref) {
                return &terminals->base[i];
            }
        }
    }

    return NULL;
}

void terminals_free(terminals_t *terminals)
{
    if (terminals == NULL)
        return;

    free(terminals->base);
    free(terminals->refs);
}
