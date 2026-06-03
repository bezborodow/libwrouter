#include "wrouter.h"
#include "dispatcher.h"
#include "router.h"
#include "lexer.h"
#include "symbol.h"
#include "params.h"
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/**
 * Dispatch.
 *
 * @param dispatcher Request dispatcher.
 * @param path Null-terminated string containing the request path.
 * @param dispatch_ctx Request dispatcher context.
 */
void wrouter_dispatch(wrouter_dispatcher_t *dispatcher, const char *path, void *dispatch_ctx)
{
    wrouter_ndispatch(dispatcher, path, strlen(path), dispatch_ctx);
}

/**
 * Dispatch, n bytes.
 *
 * @param dispatcher Request dispatcher.
 * @param path Request path.
 * @param length Length of the request path.
 * @param dispatch_ctx Request dispatcher context.
 */
void wrouter_ndispatch(wrouter_dispatcher_t *dispatcher, const char *path, size_t length,
                       void *dispatch_ctx)
{
    lexer_load(&dispatcher->lx, path, length);

    const wrouter_route_t *route = route_match(dispatcher);

    if (route == NULL)
        route = &dispatcher->router->fallback;

    if (route->handler != NULL)
        route->handler(dispatch_ctx, route->ctx, &dispatcher->params);
}

/**
 * Resolve to context.
 *
 * @param dispatcher Request dispatcher.
 * @param path Null-terminated string containing the request path.
 */
const void *wrouter_resolve(wrouter_dispatcher_t *dispatcher, const char *path)
{
    return wrouter_nresolve(dispatcher, path, strlen(path));
}

/**
 * Resolve to context, n bytes.
 *
 * @param dispatcher Request dispatcher.
 * @param path Request path.
 * @param length Length of the request path.
 */
const void *wrouter_nresolve(wrouter_dispatcher_t *dispatcher, const char *path, size_t length)
{
    lexer_load(&dispatcher->lx, path, length);

    const wrouter_route_t *route = route_match(dispatcher);

    if (route == NULL)
        return dispatcher->router->fallback.ctx;

    return route->ctx;
}

/**
 * Create a dispatcher. The dispatcher is not thread-safe.
 *
 * The mutable dispatcher holds parameters and the lexer, which are mutable.  A
 * pointer is given for the immutable router, which is not owned by the
 * dispatcher and can therefore be shared between threads.
 */
wrouter_dispatcher_t *wrouter_dispatcher_create(const wrouter_t *router)
{
    wrouter_dispatcher_t *dispatcher = calloc(1, sizeof(wrouter_dispatcher_t));
    if (dispatcher == NULL)
        return NULL;

    if (router->max_params) {
        if (params_alloc(&dispatcher->params, router->max_params))
            goto failure;
    }

    dispatcher->router = router;

    return dispatcher;

failure:
    free(dispatcher);
    return NULL;
}

/**
 * Free the dispatcher.
 *
 * This does not free the router because it may be used by another thread.
 * Instead, free the router separately after freeing all dispatchers or any
 * other resources that may be using the router.
 */
void wrouter_dispatcher_free(wrouter_dispatcher_t *dispatcher)
{
    if (dispatcher == NULL)
        return;

    params_free(&dispatcher->params);
    free(dispatcher);
}

void wrouter_dispatcher_destroy(wrouter_dispatcher_t **dpp)
{
    if (dpp == NULL || *dpp == NULL)
        return;

    wrouter_dispatcher_free(*dpp);

    *dpp = NULL;
}
