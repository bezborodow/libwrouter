#pragma once
#include "wrouter.h"
#include "lexer.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

/**
 * These are the node terminals, which correspond to a route.  The terminals
 * are referenced by the terminating node's offset.
 */
typedef struct terminals {
    wrouter_route_t *base; // Routes.
    uint16_t *refs;        // List of node offsets, being the key to the route.
    uint16_t count;        // Number of terminals.
} terminals_t;

struct wrouter_router {
    void *graph;
    symbols_t literals;
    symbols_t params;
    terminals_t terminals;
    wrouter_route_t fallback;
    size_t max_params;
    size_t num_routes;
    wrouter_reference_fn retain;
    wrouter_reference_fn release;
};

const wrouter_route_t *route_match(wrouter_dispatcher_t *d);
