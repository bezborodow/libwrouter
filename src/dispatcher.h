#pragma once
#include "router.h"
#include "lexer.h"

struct wrouter_dispatcher {
    const wrouter_t *router;
    lexer_t lx;
    wrouter_params_t params;
};
