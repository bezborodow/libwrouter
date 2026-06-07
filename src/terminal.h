#pragma once
#include "wrouter.h"
#include <stdint.h>

/**
 * Terminal dictionary.
 *
 * These are the node terminals, which correspond to a route.  The terminals
 * are referenced by the terminating node's offset.
 *
 * Refs is the keys.
 * Base is the values.
 */
typedef struct {
    uint16_t *refs;        // List of node offsets, being the key to the route.
    wrouter_route_t *base; // Routes.
    uint16_t count;        // Number of terminals.
} terminals_t;

int terminal_alloc(terminals_t *terminals, size_t n);

void terminal_append(terminals_t *terminals, uint16_t ref, const wrouter_route_t route);

wrouter_route_t *terminal_lookup(const terminals_t *terminals, uint16_t ref);

void terminals_free(terminals_t *terminals);
