#include "common.h"
#include "symbol.h"
#include "wrouter.h"
#include "segment.h"
#include "token.h"
#include "graph.h"
#include <stdint.h>
#include <stdlib.h>

segment_t *segment_create(symbol_table_t *symtbl, token_t *token, wrouter_error_t *err)
{
    // Append parameter symbol to the parameter symbol table.
    if (symtbl->count >= UINT16_MAX) {
        *err = WROUTER_ERR_OUT_OF_RANGE;
        return NULL;
    }

    const char *strptr = symbol_append(symtbl, token->ptr, token->length);
    if (strptr == NULL) {
        *err = WROUTER_ERR_NO_MEMORY;
        return NULL;
    }

    segment_t *segment = calloc(1, sizeof(segment_t));
    if (segment == NULL) {
        *err = WROUTER_ERR_NO_MEMORY;
        return NULL;
    }

    segment->str = strptr;
    segment->str_length = token->length;

    *err = WROUTER_OK;
    return segment;
}

wrouter_error_t segment_append_child(segment_t *cur, segment_t *child)
{
    if (cur->child_count >= NODE_MAX_CHILD_COUNT)
        return WROUTER_ERR_OUT_OF_RANGE;

    segment_t **new_children =
        realloc(cur->children, sizeof(segment_t *) * (cur->child_count + 1));
    if (new_children == NULL) // TODO realloc growth.
        return WROUTER_ERR_NO_MEMORY;

    cur->children = new_children;
    cur->children[cur->child_count++] = child;

    return WROUTER_OK;
}

/**
 * Find a child of a segment by token.
 */
segment_t *segment_find_child_by_token(segment_t *segment, const token_t tok)
{
    if (tok.ptr == NULL)
        return NULL;

    for (uint16_t i = 0; i < segment->child_count; i++) {
        segment_t *child = segment->children[i];

        if (token_matches_segment(tok, child))
            return child;
    }

    return NULL;
}

void segment_free(segment_t *segment)
{
    if (segment == NULL)
        return;

    switch (segment->spec_type) {
        case SPEC_WILDCARD:
            free(segment->special.wildcard);
            break;

        case SPEC_PARAM:
            segment_free(segment->special.param);
            break;

        case SPEC_NONE:
            break;
    }

    for (uint16_t i = 0; i < segment->child_count; i++)
        segment_free(segment->children[i]);

    free(segment->trailing);
    free(segment->terminal);
    free(segment->children);
    free(segment);
}

void segment_release(const segment_t *segment, const wrouter_reference_fn release)
{
    // Descend into literals.
    for (uint16_t i = 0; i < segment->child_count; i++) {
        segment_release(segment->children[i], release);
    }

    switch (segment->spec_type) {
        case SPEC_PARAM:
            // Descend into parameters.
            segment_release(segment->special.param, release);
            break;

        case SPEC_WILDCARD:
            // Release wildcard route context.
            release(segment->special.wildcard->ctx);
            break;

        case SPEC_NONE:
            break;
    }

    // Release trailing-slash route context.
    if (segment->trailing)
        release(segment->trailing->ctx);

    // Release segment terminal route context.
    if (segment->terminal)
        release(segment->terminal->ctx);
}
