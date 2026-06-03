#include "token.h"
#include <stdio.h>
#include <assert.h>

#define ASSERT_TOKEN_TYPE(tok, expected_type)                                                      \
    do {                                                                                           \
        if ((tok).type != (expected_type)) {                                                       \
            fprintf(stderr, "Expected: %s; got: %s.\n", token_ident(expected_type),                \
                    token_ident(tok.type));                                                        \
            assert(0);                                                                             \
        }                                                                                          \
    } while (0)

const char *token_ident(int type);
