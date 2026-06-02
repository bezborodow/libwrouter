#pragma once
#include "wrouter.h"
#include "lexer.h"
#include "terminal.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

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

void router_retain(wrouter_t *router, const wrouter_route_t *route);

const wrouter_route_t *route_match(wrouter_dispatcher_t *d);
