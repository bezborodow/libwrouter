#include "wrouter.h"
#include "dispatcher.h"
#include "router.h"
#include "symbol.h"
#include "params.h"
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

const wrouter_params_t *wrouter_params(const wrouter_dispatcher_t *dispatcher)
{
    return &dispatcher->params;
}

/**
 * Deep copy of parameters.
 *
 * This will perform a deep copy of parameters with the strings copied into the
 * snapshot memory region, which will be valid until the snapshot is freed.
 *
 * This function solves the problem where parameters are passed around with
 * string pointers referencing the router's symbol table and the lexer's
 * request path string. If either of these are freed before the parameters are,
 * then there will be dangling pointers and potential memory access violations.
 */
wrouter_params_snapshot_t *wrouter_params_copy(const wrouter_params_t *params)
{
    size_t size = 0;
    size_t cursor = 0;
    char *dest = NULL;

    if (params == NULL)
        return NULL;

    // Allocate snapshot.
    wrouter_params_snapshot_t *snapshot = calloc(1, sizeof(wrouter_params_snapshot_t));
    if (snapshot == NULL)
        goto failure;

    // Check for an empty parameter set. This is valid, but should return here early
    // will zero initialisation to avoid calling calloc with a size of zero.
    if (!params->count)
        return snapshot;

    if (params->base == NULL)
        goto failure;

    // Calculate size of region.
    for (uint32_t i = 0; i < params->count; i++) {
        wrouter_param_t *param = &params->base[i];

        if (param->name == NULL || param->value == NULL)
            goto failure;

        size_t name_size = strlen(param->name) + 1;
        size_t value_size = (size_t)param->length + 1;

        if (name_size > (size_t)UINT16_MAX - size)
            goto failure;
        size += name_size;

        if (value_size > (size_t)UINT16_MAX - size)
            goto failure;
        size += value_size;
    }

    // Allocate character region for storage of strings.
    // Zeroing with calloc implies null-termination of copied strings.
    snapshot->region = calloc(size, 1);
    if (snapshot->region == NULL)
        goto failure;

    // Allocate parameters.
    params_nt_alloc(&snapshot->params, params->count);
    if (snapshot->params.base == NULL)
        goto failure;

    // Copy into region.
    for (uint32_t i = 0; i < params->count; i++) {
        wrouter_param_t *src_param = &params->base[i];
        wrouter_param_nt_t *dest_param = &snapshot->params.base[i];

        // Parameter names are null-terminated in the parameter symbol table.
        dest = snapshot->region + cursor;
        strcpy(dest, src_param->name);
        dest_param->name = dest;
        cursor += strlen(src_param->name) + 1;

        // Parameter values are not null-terminated, but they will be after copy.
        dest = snapshot->region + cursor;
        memcpy(dest, src_param->value, src_param->length);
        dest_param->value = dest;
        cursor += src_param->length + 1;
    }

    snapshot->params.count = params->count;

    return snapshot;

failure:
    wrouter_snapshot_free(snapshot);
    return NULL;
}

void wrouter_snapshot_free(wrouter_params_snapshot_t *snapshot)
{
    if (snapshot == NULL)
        return;

    params_nt_free(&snapshot->params);
    free(snapshot->region);
    free(snapshot);
}

int params_alloc(wrouter_params_t *params, size_t n)
{
    params->base = calloc(n, sizeof(wrouter_param_t));

    return params->base == NULL;
}

void params_free(wrouter_params_t *params)
{
    if (params == NULL)
        return;

    free(params->base);
}

int params_nt_alloc(wrouter_params_nt_t *params, size_t n)
{
    params->base = calloc(n, sizeof(wrouter_param_nt_t));

    return params->base == NULL;
}

void params_nt_free(wrouter_params_nt_t *params)
{
    if (params == NULL)
        return;

    free(params->base);
}

const wrouter_param_t *wrouter_param(const wrouter_params_t *params, const char *name)
{
    for (uint32_t i = 0; i < params->count; i++)
        if (strcmp(params->base[i].name, name) == 0)
            return &params->base[i];

    return NULL;
}

const wrouter_param_t *wrouter_iparam(const wrouter_params_t *params, uint32_t index)
{
    if (index >= params->count)
        return NULL;

    return &params->base[index];
}
