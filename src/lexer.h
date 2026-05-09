#include <stdint.h>
#include <stddef.h>

#define TOKEN_LITERAL   0
#define TOKEN_PARAM     1
#define TOKEN_WILDCARD  2
#define TOKEN_END       255

typedef struct {
    uint8_t type;
    uint8_t length;
    uint16_t symbol;
} token_t;

typedef struct {
    const char * const *table;
    uint16_t count;
} symbol_table_t;

typedef struct {
    symbol_table_t literals;
    symbol_table_t params;
} symbol_ctx_t;

typedef struct {
    symbol_ctx_t *symbols;
    const char *cursor;
    size_t length;
} lexer_t;

void lexer_init(lexer_t *lx, const char *input, size_t length, symbol_ctx_t *symbols);

void lexer_reset(lexer_t *lx, const char *input, size_t length);

token_t lexer_next(lexer_t *lx);
