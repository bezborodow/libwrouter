#pragma once
#include "common.h"
#include "segment.h"
#include "wrouter.h"
#include "symbol.h"
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
    segment_t *head;
    segment_t *tail;
    segment_t *next;
    wrouter_route_t *terminal;
    wrouter_route_t *trailing;
    special_u special;
    special_type_t spec_type;
    uint16_t child_count;
    uint16_t str_length;
};

segment_t *segment_create(symbol_table_t *symtbl, token_t *token, wrouter_error_t *err);

wrouter_error_t segment_append_child(segment_t *cur, segment_t *child);

segment_t *segment_find_child_by_token(const segment_t *segment, const token_t token);

void segment_release(const segment_t *segment, wrouter_reference_fn release);

void segment_free(segment_t *segment);
