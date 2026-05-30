#pragma once
#include "wrouter.h"
#include "token.h"
#include <stddef.h>

typedef struct {
    const char *cursor;
    wrouter_param_syntax_t param_syntax;
} prelexer_t;

void prelexer_init(prelexer_t *lx, wrouter_param_syntax_t syntax);

void prelexer_load(prelexer_t *lx, const char *input);

token_t prelexer_next(prelexer_t *lx);
