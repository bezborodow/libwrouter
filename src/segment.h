#pragma once
#include "wrouter.h"
#include "token.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct segment segment_t;

typedef enum {
    SPEC_NONE,
    SPEC_PARAM,
    SPEC_WILDCARD,
} special_type_t;

typedef union {
    segment_t *param;
    wrouter_route_t *wildcard;
} special_u;

struct segment {
    const char *str;
    segment_t **children;
    wrouter_route_t *terminal;
    wrouter_route_t *trailing;
    special_u special;
    special_type_t spec_type;
    uint16_t child_count;
    uint16_t str_length;
};
