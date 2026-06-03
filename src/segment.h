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

typedef struct {
    wrouter_route_t route;
} wildcard_t;

typedef struct {
    wrouter_route_t route;
} trailing_t;

typedef union {
    segment_t *param;
    wildcard_t *wildcard;
} special_u;

struct segment {
    const char *str;
    wrouter_route_t route;
    segment_t **children;
    trailing_t *trailing;
    special_u special;
    special_type_t spec_type;
    uint16_t child_count;
    uint16_t str_length;
    bool terminal;
};
