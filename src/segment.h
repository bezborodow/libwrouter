#pragma once
#include "wrouter.h"
#include "common.h"
#include <stdint.h>
#include <stdbool.h>

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

segment_t *segment_find_child_by_token(segment_t *segment, const token_t token);
void segment_free(segment_t *segment);
void segment_release(wrouter_builder_t *builder, const segment_t *segment);
