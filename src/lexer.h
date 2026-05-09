#define WWR_TOKEN_LITERAL   0
#define WWR_TOKEN_PARAM     1
#define WWR_TOKEN_WILDCARD  2
#define WWR_TOKEN_EOF       255

typedef struct {
    uint8_t type;
    uint8_t length;
    uint16_t symbol;
} wwr_token_t;

typedef struct {
    const uint16_t *literal_table;
    const uint16_t *param_table;
} wwr_symbol_ctx_t;

typedef struct {
    wwr_symbol_ctx_t *symbols;
    const char *cursor;
    size_t length;
} wwr_lexer_t;

void wwr_lexer_init(wwr_lexer_t *lx,
                    const char *input,
                    size_t length,
                    wwr_symbol_ctx_t *symbols);

void wwr_lexer_reset(wwr_lexer_t *lx,
                     const char *input,
                     size_t length);

wwr_token_t wwr_lexer_next(wwr_lexer_t *lx);
