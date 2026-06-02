#pragma once
#include "wrouter.h"
#include <stdint.h>

/**
 * These are the node terminals, which correspond to a route.  The terminals
 * are referenced by the terminating node's offset.
 */
typedef struct {
    wrouter_route_t *base; // Routes.
    uint16_t *refs;        // List of node offsets, being the key to the route.
    uint16_t count;        // Number of terminals.
} terminals_t;

wrouter_route_t *terminal_lookup(const terminals_t *terminals, uint16_t ref);

void terminals_free(terminals_t *terminals);
