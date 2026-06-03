#include "builder.h"
#include "token.h"
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

token_t make_token(uint8_t type)
{
    return (token_t){ .type = type };
}

/**
 * Check if a token matches against a given segment.
 */
bool token_matches_segment(token_t tok, const segment_t *seg)
{
    return seg->str && tok.ptr && tok.length == seg->str_length &&
           strncmp(tok.ptr, seg->str, tok.length) == 0;
}
