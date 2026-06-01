#pragma once
#include "wrouter.h"
#include "lexer.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#define NODE_FLAG_TERMINAL 1
#define NODE_FLAG_HAS_PARAM 2
#define NODE_FLAG_HAS_WILDCARD 4
#define NODE_FLAG_HAS_TRAILING 8

typedef struct node {
    uint8_t literals; // Number of literal edges.
    uint8_t flags;
} node_t;

typedef struct edge {
    uint16_t symbol;
    uint16_t next;
} edge_t;

_Static_assert(sizeof(node_t) % _Alignof(edge_t) == 0, "Node size breaks edge alignment.");
_Static_assert(sizeof(edge_t) % _Alignof(node_t) == 0, "Edge size breaks node alignment.");

typedef struct symbols {
    const char **base;
    char *region;
    uint32_t count;
} symbols_t;

/**
 * These are the node terminals, which correspond to a route.  The terminals
 * are referenced by the terminating node's offset.
 */
typedef struct terminals {
    struct route *base; // Routes.
    uint16_t *refs;     // List of node offsets, being the key to the route.
    uint16_t count;     // Number of terminals.
} terminals_t;

struct router {
    void *graph;
    symbols_t literals;
    symbols_t params;
    terminals_t terminals;
    struct route fallback;
    size_t max_params;
    size_t num_routes;
    wrouter_reference_fn retain;
    wrouter_reference_fn release;
};

struct router_options {
    struct route fallback;
    wrouter_param_syntax_t param_syntax;
};

size_t graph_offset(const void *graph, const void *entry);

const struct route *route_match(struct dispatcher *d);
