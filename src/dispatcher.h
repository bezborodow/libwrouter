#include "router.h"
#include "lexer.h"

#ifndef WROUTER_DISPATCHER_H
#define WROUTER_DISPATCHER_H

struct dispatcher {
    const struct router *router;
    lexer_t lx;
    struct params params;
};

#endif
