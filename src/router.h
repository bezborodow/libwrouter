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
    uint8_t literals; // Numbers of literal edges.
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

/*
_Static_assert(sizeof(node_t) % _Alignof(edge_t) == 0, "Node size breaks edge alignment.");

_Static_assert(_Alignof(edge_t) <= _Alignof(node_t),
               "Edge alignment requirement not satisfied by node alignment.");
               */

typedef struct symbols {
    const char **base;
    char *region;
    uint32_t count;
} symbols_t;

typedef struct terminals {
    struct route *base;
    uint32_t count;
} terminals_t;

struct router {
    void *graph;
    symbols_t literals;
    symbols_t params;
    terminals_t terminals;
};

#endif
