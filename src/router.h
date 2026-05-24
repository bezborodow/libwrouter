#include "wrouter.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#ifndef WROUTER_ROUTER_H
#define WROUTER_ROUTER_H

#define NODE_FLAG_TERMINAL 1
#define NODE_FLAG_HAS_PARAM 2
#define NODE_FLAG_HAS_WILDCARD 4

typedef struct {
    const char *name;
    const char *value;
} param_t;

struct params {
    const param_t *base;
    uint32_t count;
};

typedef struct node {
    uint8_t literals; // Number of literal edges.
    uint8_t flags;
} node_t;

typedef struct edge {
    uint16_t symbol;
    uint16_t next;
} edge_t;

struct route {
    wrouter_handler_fn handler;
    void *ctx;
};

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
};

#endif
