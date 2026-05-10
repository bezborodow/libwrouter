#ifndef WROUTER_H
#define WROUTER_H

/**
 * Route Handler.
 */
typedef void (*wrouter_handler_t)(void *ctx, const char *const *params);

/**
 * Route builder.
 */
typedef struct builder_t wrouter_builder_t;
typedef enum {
    WROUTER_SYNTAX_COLON, // :id
    WROUTER_SYNTAX_BRACE, // {id}
    WROUTER_SYNTAX_ANGLE  // <id>
} wrouter_param_syntax_t;

/**
 * Router.
 */
typedef struct router_t wrouter_t;

/**
 * Builder API.
 */
wrouter_builder_t *wrouter_builder_create(wrouter_param_syntax_t param_syntax);

int wrouter_add_route(wrouter_builder_t *builder, const char *pattern, wrouter_handler_t handler,
                      void *handler_ctx);

/**
 * wrouter_t *router = NULL;
 * int err = wrouter_compile(builder, &router);
 */
int wrouter_compile(const wrouter_builder_t *builder, wrouter_t **router);

void wrouter_builder_free(wrouter_builder_t *builder);

/**
 * Router API.
 */
int wrouter_match(const wrouter_t *router, const char *path, wrouter_handler_t *out_handler,
                  void **out_ctx, const char ***out_params);

int wrouter_route_count(const wrouter_t *router);

void wrouter_reset(wrouter_t *router);

void wrouter_free(wrouter_t *router);

#endif
