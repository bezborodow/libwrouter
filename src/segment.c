#include "wrouter.h"
#include "segment.h"
#include "token.h"
#include <stdint.h>
#include <stdlib.h>

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
