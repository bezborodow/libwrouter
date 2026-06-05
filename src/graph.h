#pragma once
#include "builder.h"
#include <stddef.h>
#include <stdlib.h>

// Maximum literal number of children per node.  Node data has 12 bits reserved
// for a node count. That allows a maximum of 4095 literal child edges per
// node. See node_t.data.
#define NODE_LITERALS_MASK ((size_t)((1 << 12) - 1))
#define NODE_MAX_CHILD_COUNT NODE_LITERALS_MASK

// The most-significant 4 bits of the 16 bit node data are the following flags:
#define NODE_FLAG_TERMINAL (1 << 12)
#define NODE_FLAG_HAS_PARAM (1 << 13)
#define NODE_FLAG_HAS_WILDCARD (1 << 14)
#define NODE_FLAG_HAS_TRAILING (1 << 15)

typedef struct {
    size_t nodes;
    size_t edges;
    size_t symbolic_edges;
    size_t terminals;
    size_t size;
    size_t param_depth;
    size_t max_params;
} graph_stats_t;

typedef struct {
    uint16_t data;
} node_t;

typedef struct {
    uint16_t symbol;
    uint16_t next;
} edge_t;

_Static_assert(sizeof(node_t) % _Alignof(edge_t) == 0, "Node size breaks edge alignment.");
_Static_assert(sizeof(edge_t) % _Alignof(node_t) == 0, "Edge size breaks node alignment.");

#define GRAPH_ADDR_UNIT 2
#define GRAPH_ADDR_SHIFT 1

_Static_assert(sizeof(node_t) % GRAPH_ADDR_UNIT == 0,
               "INVALID GRAPH LAYOUT: node_t size incompatible with address unit encoding.");

_Static_assert(sizeof(edge_t) % GRAPH_ADDR_UNIT == 0,
               "INVALID GRAPH LAYOUT: edge_t size incompatible with address unit encoding.");
_Static_assert((1u << GRAPH_ADDR_SHIFT) == GRAPH_ADDR_UNIT,
               "GRAPH_ADDR_SHIFT must match GRAPH_ADDR_UNIT (power-of-two encoding).");
_Static_assert((GRAPH_ADDR_UNIT & (GRAPH_ADDR_UNIT - 1)) == 0,
               "GRAPH_ADDR_UNIT must be power of two.");

// Max graph bytes. Based on the transition offset (edge_t.next).
#define GRAPH_CAPACITY_BYTES ((size_t)(UINT16_MAX + 1) << GRAPH_ADDR_SHIFT)

const edge_t *node_edge_base(const node_t *node);

const node_t *next_node(const uint8_t *graph, const edge_t *edge);

size_t graph_offset(const void *graph, const void *entry);

void graph_stats(const segment_t *segment, graph_stats_t *stats);

node_t *graph_compile(wrouter_t *router, const segment_t *segment, size_t *cursor);

void *graph_append(void *g, size_t *cursor, size_t size, size_t align);

uintptr_t graph_align_up(size_t cursor, size_t align);
