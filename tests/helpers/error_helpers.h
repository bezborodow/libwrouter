#include "wrouter.h"
#include <stdio.h>

#define ASSERT_ERROR(expr, expected_err)                                                           \
    do {                                                                                           \
        wrouter_error_t _got = (expr);                                                             \
        if (_got != (expected_err)) {                                                              \
            fprintf(stderr, "Expected error: %s got: %s\n", wrouter_strerror(expected_err),        \
                    wrouter_strerror(_got));                                                       \
            assert(expected_err == _got);                                                          \
        }                                                                                          \
    } while (0)
