#include "wrouter.h"
#include <stddef.h>
#include <stdint.h>

#ifndef WROUTER_ROUTER_H
#define WROUTER_ROUTER_H

typedef struct node {
    uint32_t special; // Param or wildcard.
    uint32_t terminal;
    uint32_t edge_base;
    uint16_t edge_count;
    uint16_t flags;
} node_t;

typedef struct edge {
    uint32_t next;
    uint32_t symbol;
} edge_t;

typedef struct terminal {
    //handler_fn fn;
    void *ctx;
    uint32_t metadata;
} terminal_t;

_Static_assert(sizeof(node_t) % _Alignof(edge_t) == 0,
               "Node size breaks edge alignment.");

_Static_assert(_Alignof(edge_t) <= _Alignof(node_t),
               "Edge alignment requirement not satisfied by node alignment.");

struct router {};

int router_init(struct router *r,
                size_t node_count,
                size_t edge_count)
{
    /*
    size_t node_bytes = sizeof(node_t) * node_count;
    size_t edge_bytes = sizeof(edge_t) * edge_count;

    uint8_t *mem = malloc(node_bytes + edge_bytes);
    if (!mem) return -1;

    r->nodes = (node_t *)mem;
    r->edges = (edge_t *)(mem + node_bytes);
    r->mem = mem;

    r->node_count = node_count;
    r->edge_count = edge_count;

    r->terminals = malloc(sizeof(terminal_t) * term_count);
    */

    return 0;
}

#endif
