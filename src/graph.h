#pragma once
#include "builder.h"
#include <stddef.h>
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

size_t graph_offset(const void *graph, const void *entry);
void graph_stats(const segment_t *seg, graph_stats_t *stats);
node_t *graph_compile(wrouter_t *router, const segment_t *segment, size_t *cursor);
