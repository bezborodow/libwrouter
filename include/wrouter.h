typedef struct wrouter wrouter_t;
typedef struct wrouter_route wrouter_route_t;

typedef void (*wrouter_handler_t)(void *ctx, const char *const *params);

typedef enum {
    WROUTER_SYNTAX_COLON,   // :id
    WROUTER_SYNTAX_BRACE,   // {id}
    WROUTER_SYNTAX_ANGLE    // <id>
} wwr_syntax_t;

wrouter_t *wrouter_create(void);
void wrouter_destroy(wrouter_t *router);

int wrouter_add_route(wrouter_t *router,
                      const char *pattern,
                      wrouter_handler_t handler,
                      void *handler_ctx);

int wrouter_match(const wrouter_t *router,
                  const char *path,
                  wrouter_handler_t *out_handler,
                  void **out_ctx,
                  const char ***out_params);

int wrouter_route_count(const wrouter_t *router);
void wrouter_reset(wrouter_t *router);
