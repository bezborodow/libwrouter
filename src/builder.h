#include "symbol.h"
#include "token.h"
#include "router.h"

#ifndef WROUTER_BUILDER_H
#define WROUTER_BUILDER_H

typedef struct segment segment_t;

typedef enum {
    SPEC_PARAM,
    SPEC_WILDCARD,
} special_type_t;

typedef struct wildcard {
    struct route route;
} wildcard_t;

typedef union {
    segment_t *param_segment;
    wildcard_t *wildcard;
} special_union_t;

typedef struct special {
    segment_t *children;
    char *str;
    size_t length;
    special_type_t type;
} special_t;

struct segment {
    char *str;
    segment_t *children;
    special_t special;
    size_t length;
    struct route route;
};

typedef struct tree {
    segment_t root;
} tree_t;

typedef struct {
    pretoken_t *tokens;
    size_t token_count;
} preroute_t;

typedef struct {
    preroute_t *base;
    size_t count;
    size_t capacity;
} preroute_table_t;

struct builder {
    wrouter_param_syntax_t param_syntax;
    preroute_table_t routes;
    arena_t arena;
    symbol_table_t literals;
    symbol_table_t params;
};

#endif
