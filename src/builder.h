#pragma once
#include "symbol.h"
#include "router.h"
#include "wrouter.h"
#include "token.h"
#include "segment.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    token_t *tokens;
    size_t token_count;
} preroute_t;

typedef struct {
    preroute_t *base;
    size_t count;
    size_t capacity;
} preroute_table_t;

struct wrouter_builder {
    segment_t *root;
    preroute_table_t routes;
    arena_t arena;
    symbol_table_t literals;
    symbol_table_t params;
    wrouter_route_t fallback;
    wrouter_reference_fn retain;
    wrouter_reference_fn release;
    wrouter_param_syntax_t param_syntax;
    bool corrupted;
};
