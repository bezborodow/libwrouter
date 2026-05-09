#include "wrouter.h"
#include "token.h"
#include <stdint.h>
#include <stddef.h>

#ifndef WROUTER_PRELEXER_H
#define WROUTER_PRELEXER_H

typedef struct {
    uint8_t type;
    uint8_t length;
} pretoken_t;

typedef struct {
    const char *cursor;
    wrouter_param_syntax_t param_syntax;
} prelexer_t;

void prelexer_init(prelexer_t *lx, wrouter_param_syntax_t syntax);

void prelexer_load(prelexer_t *lx, const char *input);

pretoken_t prelexer_next(prelexer_t *lx);

#endif
