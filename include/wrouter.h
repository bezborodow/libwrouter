#pragma once
#include <stdint.h>
#include <stddef.h>

/**
 * Router.
 */
typedef struct router wrouter_t;
typedef struct builder wrouter_builder_t;
typedef struct dispatcher wrouter_dispatcher_t;

typedef struct param {
    const char *name;
    const char *value;
    uint16_t length;
} wrouter_param_t;

/**
 * Null-terminated params.
 */
typedef struct param_nt {
    const char *name;
    const char *value;
} wrouter_param_nt_t;

typedef struct params {
    wrouter_param_t *base;
    uint32_t count;
} wrouter_params_t;

typedef struct params_nt {
    wrouter_param_nt_t *base;
    uint32_t count;
} wrouter_params_nt_t;

typedef struct params_snapshot {
    wrouter_params_nt_t params;
    char *region;
} wrouter_params_snapshot_t;

typedef void (*wrouter_handler_fn)(void *dispatch_ctx, const void *route_ctx,
                                   const wrouter_params_t *params);

typedef void (*wrouter_reference_fn)(const void *ctx);

typedef struct route {
    wrouter_handler_fn handler;
    const void *ctx;
} wrouter_route_t;

typedef enum {
    WROUTER_SYNTAX_COLON, // :id
    WROUTER_SYNTAX_BRACE, // {id}
    WROUTER_SYNTAX_ANGLE, // <id>
} wrouter_param_syntax_t;

typedef struct wrouter_options {
    wrouter_handler_fn fallback_handler;
    const void *fallback_ctx; // TODO const is new change. Make sure it doesn't break anything.
    wrouter_param_syntax_t param_syntax;
    wrouter_reference_fn retain;
    wrouter_reference_fn release;
} wrouter_options_t;

wrouter_builder_t *wrouter_builder_create(const wrouter_options_t options);

int wrouter_add_route(wrouter_builder_t *builder, const char *pattern, wrouter_route_t route);
int wrouter_add_handler(wrouter_builder_t *builder, const char *pattern,
                        wrouter_handler_fn handler);
int wrouter_add_handler_ctx(wrouter_builder_t *builder, const char *pattern,
                            wrouter_handler_fn handler, const void *ctx);
int wrouter_add_context(wrouter_builder_t *builder, const char *pattern, const void *ctx);

wrouter_t *wrouter_compile(const wrouter_builder_t *builder);

wrouter_dispatcher_t *wrouter_dispatcher_create(const wrouter_t *router);

void wrouter_dispatcher_free(wrouter_dispatcher_t *dispatcher);

void wrouter_builder_free(wrouter_builder_t *builder);

void wrouter_dispatch(wrouter_dispatcher_t *dispatcher, const char *path, void *dispatch_ctx);
void wrouter_ndispatch(wrouter_dispatcher_t *dispatcher, const char *path, size_t length,
                       void *dispatch_ctx);

const void *wrouter_nresolve(wrouter_dispatcher_t *dispatcher, const char *path, size_t length);
const void *wrouter_resolve(wrouter_dispatcher_t *dispatcher, const char *path);
const wrouter_params_t *wrouter_params(const wrouter_dispatcher_t *dispatcher);

wrouter_params_snapshot_t *wrouter_params_copy(const wrouter_params_t *params);
void wrouter_snapshot_free(wrouter_params_snapshot_t *snapshot);

size_t wrouter_route_count(const wrouter_t *router);

void wrouter_free(wrouter_t *router);
