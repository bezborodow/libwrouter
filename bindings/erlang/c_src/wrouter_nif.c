#include "wrouter.h"

#include <erl_nif.h>
#include <string.h>

typedef struct {
    ErlNifEnv *env;
    ERL_NIF_TERM term;
    unsigned int refs;
} route_context_t;

typedef struct {
    wrouter_t *router;
} router_resource_t;

static ErlNifResourceType *router_resource_type = NULL;
static ERL_NIF_TERM atom_error;
static ERL_NIF_TERM atom_not_found;
static ERL_NIF_TERM atom_ok;

static void route_context_free(route_context_t *ctx)
{
    if (ctx == NULL)
        return;

    if (ctx->env != NULL)
        enif_free_env(ctx->env);

    enif_free(ctx);
}

static route_context_t *route_context_create(ERL_NIF_TERM term)
{
    route_context_t *ctx = enif_alloc(sizeof(*ctx));
    if (ctx == NULL)
        return NULL;

    ctx->env = enif_alloc_env();
    if (ctx->env == NULL) {
        enif_free(ctx);
        return NULL;
    }

    ctx->term = enif_make_copy(ctx->env, term);
    ctx->refs = 0;

    return ctx;
}

static void route_context_retain(const void *ptr)
{
    route_context_t *ctx = (route_context_t *)ptr;
    if (ctx != NULL)
        ctx->refs++;
}

static void route_context_release(const void *ptr)
{
    route_context_t *ctx = (route_context_t *)ptr;
    if (ctx == NULL)
        return;

    if (ctx->refs > 0)
        ctx->refs--;

    if (ctx->refs == 0)
        route_context_free(ctx);
}

static int make_binary(ErlNifEnv *env, const char *src, size_t len, ERL_NIF_TERM *term)
{
    unsigned char *dst = enif_make_new_binary(env, len, term);

    if (dst == NULL)
        return 0;

    memcpy(dst, src, len);
    return 1;
}

static ERL_NIF_TERM make_error(ErlNifEnv *env, const char *reason)
{
    ERL_NIF_TERM reason_bin;

    if (!make_binary(env, reason, strlen(reason), &reason_bin))
        return enif_make_badarg(env);

    return enif_make_tuple2(env, atom_error, reason_bin);
}

static int copy_iolist_to_cstr(ErlNifEnv *env, ERL_NIF_TERM term, char **out, size_t *out_len)
{
    ErlNifBinary bin;
    char *str;

    if (!enif_inspect_iolist_as_binary(env, term, &bin))
        return 0;

    str = enif_alloc(bin.size + 1);
    if (str == NULL)
        return 0;

    memcpy(str, bin.data, bin.size);
    str[bin.size] = '\0';

    *out = str;
    if (out_len != NULL)
        *out_len = bin.size;
    return 1;
}

static ERL_NIF_TERM make_params(ErlNifEnv *env, const wrouter_params_t *params)
{
    ERL_NIF_TERM map = enif_make_new_map(env);

    for (uint32_t i = 0; i < params->count; i++) {
        const wrouter_param_t *param = &params->base[i];
        ERL_NIF_TERM name;
        ERL_NIF_TERM value;

        if (!make_binary(env, param->name, strlen(param->name), &name))
            return enif_make_badarg(env);

        if (!make_binary(env, param->value, param->length, &value))
            return enif_make_badarg(env);

        if (!enif_make_map_put(env, map, name, value, &map))
            return enif_make_badarg(env);
    }

    return map;
}

static void router_resource_dtor(ErlNifEnv *env, void *obj)
{
    (void)env;

    router_resource_t *resource = obj;
    if (resource->router != NULL) {
        wrouter_free(resource->router);
        resource->router = NULL;
    }
}

static ERL_NIF_TERM new_nif(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
    ERL_NIF_TERM list;
    ERL_NIF_TERM result;
    wrouter_builder_t *builder = NULL;
    wrouter_t *router = NULL;
    router_resource_t *resource = NULL;
    wrouter_error_t err = WROUTER_OK;
    wrouter_options_t options = { 0 };

    if (argc != 1 || !enif_is_list(env, argv[0]))
        return enif_make_badarg(env);

    options.retain = route_context_retain;
    options.release = route_context_release;

    builder = wrouter_builder_create(options);
    if (builder == NULL)
        return make_error(env, wrouter_strerror(WROUTER_ERR_NO_MEMORY));

    list = argv[0];
    while (!enif_is_empty_list(env, list)) {
        ERL_NIF_TERM head;
        ERL_NIF_TERM tail;
        const ERL_NIF_TERM *tuple;
        int arity;
        char *pattern = NULL;
        route_context_t *ctx = NULL;

        if (!enif_get_list_cell(env, list, &head, &tail) ||
            !enif_get_tuple(env, head, &arity, &tuple) || arity != 2) {
            wrouter_builder_free(builder);
            return enif_make_badarg(env);
        }

        if (!copy_iolist_to_cstr(env, tuple[0], &pattern, NULL)) {
            wrouter_builder_free(builder);
            return enif_make_badarg(env);
        }

        ctx = route_context_create(tuple[1]);
        if (ctx == NULL) {
            enif_free(pattern);
            wrouter_builder_free(builder);
            return make_error(env, wrouter_strerror(WROUTER_ERR_NO_MEMORY));
        }

        err = wrouter_add_context(builder, pattern, ctx);
        enif_free(pattern);

        if (err != WROUTER_OK) {
            route_context_free(ctx);
            wrouter_builder_free(builder);
            return make_error(env, wrouter_strerror(err));
        }

        list = tail;
    }

    router = wrouter_consume(&builder, &err);
    if (err != WROUTER_OK)
        return make_error(env, wrouter_strerror(err));

    resource = enif_alloc_resource(router_resource_type, sizeof(*resource));
    if (resource == NULL) {
        wrouter_free(router);
        return make_error(env, wrouter_strerror(WROUTER_ERR_NO_MEMORY));
    }

    resource->router = router;

    result = enif_make_resource(env, resource);
    enif_release_resource(resource);

    return enif_make_tuple2(env, atom_ok, result);
}

static ERL_NIF_TERM resolve_nif(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
    router_resource_t *resource;
    wrouter_dispatcher_t *dispatcher;
    char *path = NULL;
    size_t path_len = 0;
    const route_context_t *ctx;
    ERL_NIF_TERM context;
    ERL_NIF_TERM params;

    if (argc != 2 || !enif_get_resource(env, argv[0], router_resource_type, (void **)&resource) ||
        !copy_iolist_to_cstr(env, argv[1], &path, &path_len))
        return enif_make_badarg(env);

    dispatcher = wrouter_dispatcher_create(resource->router);
    if (dispatcher == NULL) {
        enif_free(path);
        return make_error(env, wrouter_strerror(WROUTER_ERR_NO_MEMORY));
    }

    ctx = wrouter_nresolve(dispatcher, path, path_len);

    if (ctx == NULL) {
        enif_free(path);
        wrouter_dispatcher_free(dispatcher);
        return atom_not_found;
    }

    context = enif_make_copy(env, ctx->term);
    params = make_params(env, wrouter_params(dispatcher));
    enif_free(path);
    wrouter_dispatcher_free(dispatcher);

    return enif_make_tuple3(env, atom_ok, context, params);
}

static ERL_NIF_TERM route_count_nif(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
    router_resource_t *resource;

    if (argc != 1 || !enif_get_resource(env, argv[0], router_resource_type, (void **)&resource))
        return enif_make_badarg(env);

    return enif_make_uint64(env, wrouter_route_count(resource->router));
}

static int load(ErlNifEnv *env, void **priv, ERL_NIF_TERM info)
{
    (void)priv;
    (void)info;

    router_resource_type =
        enif_open_resource_type(env, NULL, "wrouter_router", router_resource_dtor,
                                ERL_NIF_RT_CREATE | ERL_NIF_RT_TAKEOVER, NULL);

    if (router_resource_type == NULL)
        return -1;

    atom_error = enif_make_atom(env, "error");
    atom_not_found = enif_make_atom(env, "not_found");
    atom_ok = enif_make_atom(env, "ok");

    return 0;
}

static ErlNifFunc funcs[] = {
    { "new_nif", 1, new_nif, 0 },
    { "resolve_nif", 2, resolve_nif, 0 },
    { "route_count_nif", 1, route_count_nif, 0 },
};

ERL_NIF_INIT(wrouter, funcs, load, NULL, NULL, NULL)
