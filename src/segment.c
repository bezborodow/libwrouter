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
    // Create segment.
    segment_t *segment = calloc(1, sizeof(segment_t));
    if (segment == NULL) {
        *err = WROUTER_ERR_NO_MEMORY;
        return NULL;
    }

    // Append parameter symbol to the parameter symbol table.
    segment->str = symbol_append(symtbl, token->ptr, token->length, err);
    segment->str_length = token->length;
    if (segment->str == NULL || *err) {
        free(segment);
        return NULL;
    }

    return segment;
}

wrouter_error_t segment_append_child(segment_t *cur, segment_t *child)
{
    if (cur->child_count >= NODE_MAX_CHILD_COUNT)
        return WROUTER_ERR_OUT_OF_RANGE;

    if (cur->head == NULL) {
        cur->head = child;
        cur->tail = child;
    } else {
        cur->tail->next = child;
        cur->tail = child;
    }

    cur->child_count++;

    return WROUTER_OK;
}

/**
 * Find a child of a segment by token.
 */
segment_t *segment_find_child_by_token(const segment_t *segment, const token_t tok)
{
    if (tok.ptr == NULL)
        return NULL;


    for (segment_t *child = segment->head; child; child = child->next) {
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

    // Free child segments.
    segment_t *child = segment->head;
    while (child) {
        segment_t *next = child->next;

        segment_free(child);

        child = next;
    }

    free(segment->trailing);
    free(segment->terminal);
    free(segment);
}

void segment_release(const segment_t *segment, const wrouter_reference_fn release)
{
    // Descend child segments.
    for (const segment_t *child = segment->head; child; child = child->next) {
        segment_release(child, release);
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
