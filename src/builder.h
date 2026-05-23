#include "symbol.h"
#include "token.h"
#include "router.h"
#include <stdint.h>

#ifndef WROUTER_BUILDER_H
#define WROUTER_BUILDER_H

typedef struct segment segment_t;

typedef enum {
    SPEC_NONE,
    SPEC_PARAM,
    SPEC_WILDCARD,
} special_type_t;

typedef struct wildcard {
    struct route route;
} wildcard_t;

typedef union {
    segment_t *param;
    wildcard_t *wildcard;
} special_u;

struct segment {
    char *str;
    struct route route;
    segment_t **children;
    special_u special;
    special_type_t spec_type;
    uint16_t child_count;
    uint16_t str_length;
};

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
    segment_t *root;
    preroute_table_t routes;
    arena_t arena;
    symbol_table_t literals;
    symbol_table_t params;
    wrouter_param_syntax_t param_syntax;
};

#endif
