/*
 * ============================
 * wrouter lexer constraints
 * ============================
 *
 * This lexer is designed for a static, high-performance routing system.
 * It is intentionally minimal and makes strong assumptions about usage.
 *
 * ----------------------------
 * Core assumptions
 * ----------------------------
 *
 * 1. Forward-only parsing.
 *    - The lexer is strictly single-pass.
 *
 * 2. Bounded input.
 *    - Input is provided as (pointer + length).
 *
 * 3. Static route set.
 *    - Route tables are built at startup, and are immutable during runtime
 *      unless rebuilt.  That is, any route change implies full rebuild.
 *
 * 4. Symbol interning is pre-initialised.
 *    - The lexer resolves identifiers using a prebuilt symbol context.
 *    - Symbol tables are immutable during lexer lifetime.
 *
 * 5. Integer token output.
 *    - Tokens are compact and numeric (no string ownership in tokens).
 *    - Token fields represent:
 *        - type (literal / param / wildcard / eof)
 *        - length (segment length in input buffer)
 *        - symbol (interned ID where applicable)
 *
 * 6. Zero-copy design.
 *    - No string allocations occur in the lexer.
 *    - Tokens reference the original input buffer via cursor progression.
 *
 * 7. No HTTP semantics/
 *    - Lexer operates only on request-target (path component).
 *    - HTTP framing, methods, headers, and protocol parsing are external.
 *
 * ----------------------------
 * Correct usage model
 * ----------------------------
 *
 * 1. HTTP layer extracts request-target.
 * 2. Lexer tokenises request-target in one pass.
 * 3. Parser/matcher consumes tokens sequentially.
 * 4. Symbol IDs map directly into precompiled route tables.
 *
 * ============================
 */

#include "lexer.h"
#include "symbol.h"
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

void lexer_init(lexer_t *lx, const char *input, size_t length, symbol_ctx_t *symbols)
{
    lx->symbols = symbols;
    lx->cursor = input;
    lx->length = length;
}

token_t lexer_next(lexer_t *lx)
{
    const char *p = lx->cursor;
    const char *end = lx->cursor + lx->length;
    const char *const *const *se;

    token_t tok = { 0 };
    tok.type = TOKEN_END;
    tok.length = 0;
    tok.symbol = 0;

    while (p < end) {
        char c = *p;

        // Skip separators.
        if (c == '/') {
            p++;
            continue;
        }

        const char *start;
        for (start = p; p < end && *p != '/'; p++)
            ;

        size_t len = (size_t)(p - start);

        lx->cursor = p;
        break;
    }

    if (p >= end) {
        lx->cursor = p;
        return tok;
    }

    return tok;
}
