#pragma once
#include "builder.h"
#include <stddef.h>
#include <stdlib.h>

// Maximum literal number of children per node.  Node data has 12 bits reserved
// for a node count. That allows a maximum of 4095 literal child edges per
// node.
#define NODE_LITERALS_MASK ((size_t)((1 << 12) - 1))
#define NODE_MAX_CHILD_COUNT NODE_LITERALS_MASK

// The most-significant 4 bits of the 16 bit node data are the following flags:
#define NODE_FLAG_TERMINAL (1 << 12)
#define NODE_FLAG_HAS_PARAM (1 << 13)
#define NODE_FLAG_HAS_WILDCARD (1 << 14)
#define NODE_FLAG_HAS_TRAILING (1 << 15)

typedef struct {
    size_t terminals;
    size_t size;
    size_t param_depth;
    size_t max_params;
} graph_stats_t;

void graph_stats(const segment_t *segment, graph_stats_t *stats);

uint16_t *graph_compile(wrouter_t *router, const segment_t *segment, uint16_t **pcur);
