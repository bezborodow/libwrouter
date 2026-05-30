#pragma once
#include "router.h"
#include "lexer.h"

struct dispatcher {
    const struct router *router;
    lexer_t lx;
    struct params params;
};
