#pragma once
#include "symbol.h"
#include "token.h"
#include "router.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct segment segment_t;

typedef enum {
    SPEC_NONE,
    SPEC_PARAM,
    SPEC_WILDCARD,
} special_type_t;

typedef struct wildcard {
    struct route route;
} wildcard_t;

typedef struct trailing {
    struct route route;
} trailing_t;

typedef union {
    segment_t *param;
    wildcard_t *wildcard;
} special_u;

struct segment {
    const char *str;
    struct route route;
    segment_t **children;
    trailing_t *trailing;
    special_u special;
    special_type_t spec_type;
    uint16_t child_count;
    uint16_t str_length;
    bool terminal;
};

typedef struct {
    token_t *tokens;
    size_t token_count;
} preroute_t;

typedef struct {
    preroute_t *base;
    size_t count;
    size_t capacity;
} preroute_table_t;

struct builder {
    segment_t *root;
    preroute_table_t routes;
    arena_t arena;
    symbol_table_t literals;
    symbol_table_t params;
    struct route fallback;
    wrouter_reference_fn retain;
    wrouter_reference_fn release;
    wrouter_param_syntax_t param_syntax;
};

typedef struct graph_stats {
    size_t nodes;
    size_t edges;
    size_t symbolic_edges;
    size_t terminals;
    size_t size;
    size_t param_depth;
    size_t max_params;
} graph_stats_t;

void graph_stats(const segment_t *segment, graph_stats_t *stats);
