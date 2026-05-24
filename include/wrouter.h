#include <stdint.h>
#include <stddef.h>

#ifndef WROUTER_H
#define WROUTER_H

/**
 * Router.
 */
typedef struct router wrouter_t;
typedef struct route wrouter_route_t;
typedef struct builder wrouter_builder_t;
typedef struct params wrouter_params_t;

typedef void (*wrouter_handler_fn)(void *dispatch_ctx, void *route_ctx,
                                   const wrouter_params_t *params);

typedef enum {
    WROUTER_SYNTAX_COLON, // :id
    WROUTER_SYNTAX_BRACE, // {id}
    WROUTER_SYNTAX_ANGLE, // <id>
} wrouter_param_syntax_t;

wrouter_builder_t *wrouter_builder_create(wrouter_param_syntax_t param_syntax);

int wrouter_add_route(wrouter_builder_t *builder, const char *pattern, wrouter_route_t route);

wrouter_t *wrouter_compile(const wrouter_builder_t *builder);

void wrouter_builder_free(wrouter_builder_t *builder);

void wrouter_dispatch(const wrouter_t *router, const char *path, void *dispatch_ctx);
void wrouter_ndispatch(const wrouter_t *router, const char *path, size_t length,
                       void *dispatch_ctx);

int wrouter_route_count(const wrouter_t *router);

void wrouter_free(wrouter_t *router);

#endif
