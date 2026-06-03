#pragma once
#include "common.h"
#include "segment.h"
#include <stdint.h>
#include <stdbool.h>

#define TOKEN_ILLEGAL 0
#define TOKEN_LITERAL 1
#define TOKEN_PARAM 2
#define TOKEN_WILDCARD 4
#define TOKEN_TRAILING 128
#define TOKEN_END 255

typedef struct token {
    const char *ptr;
    uint16_t length;
    uint8_t type;
} token_t;

token_t make_token(uint8_t type);

bool token_matches_segment(token_t tok, const segment_t *segment);
