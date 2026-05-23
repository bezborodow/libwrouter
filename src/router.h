#include "wrouter.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#ifndef WROUTER_ROUTER_H
#define WROUTER_ROUTER_H

typedef struct {
    const char *name;
    const char *value;
} param_t;

struct params {
    const param_t *base;
    uint32_t count;
};

typedef struct node {
    uint32_t special; // Param or wildcard.
    uint32_t route;
    uint32_t edge_base;
    uint16_t edge_count;
    uint16_t flags;
} node_t;

typedef struct edge {
    uint32_t next;
    uint32_t symbol;
} edge_t;

struct route {
    wrouter_handler_fn handler;
    void *ctx;
};

_Static_assert(sizeof(node_t) % _Alignof(edge_t) == 0, "Node size breaks edge alignment.");

_Static_assert(_Alignof(edge_t) <= _Alignof(node_t),
               "Edge alignment requirement not satisfied by node alignment.");

typedef struct symbols {
    const char **base;
    uint32_t count;
} symbols_t;

typedef struct terminals {
    wrouter_route_t *base;
    uint32_t count;
} terminals_t;

typedef struct graph {
    uint8_t *base;
    uint32_t node_count;
    uint32_t edge_count;
    uint32_t size;
} graph_t;

struct router {
    symbols_t symbols;
    graph_t graph;
    terminals_t terminals;
};

/*
int router_init(struct router *r, size_t node_count, size_t edge_count, size_t term_count)
{
    size_t node_bytes = sizeof(node_t) * node_count;
    size_t edge_bytes = sizeof(edge_t) * edge_count;

    uint8_t *graph = malloc(node_bytes + edge_bytes);
    if (!graph)
        return -1;

    r->nodes = (node_t *)mem;
    r->edges = (edge_t *)(mem + node_bytes);
    r->mem = mem;

    r->node_count = node_count;
    r->edge_count = edge_count;

    r->terminals = malloc(sizeof(terminal_t) * term_count);

    return 0;
}
*/

#endif
