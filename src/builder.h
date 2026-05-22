#include "symbol.h"
#include "token.h"

#ifndef WROUTER_BUILDER_H
#define WROUTER_BUILDER_H

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
    symbol_ctx_t symctx;
};

#endif
