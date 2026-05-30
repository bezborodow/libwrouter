#pragma once
#include <stdint.h>

#define TOKEN_ILLEGAL 0
#define TOKEN_LITERAL 1
#define TOKEN_PARAM 2
#define TOKEN_WILDCARD 3
#define TOKEN_END 255

typedef struct {
    const char *ptr;
    uint16_t length;
    uint8_t type;
} token_t;
