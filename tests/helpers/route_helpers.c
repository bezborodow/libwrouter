#include "graph.h"
#include "symbol.h"
#include "wrouter.h"
#include "builder.h"
#include "router.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum graph_entry {
    G_NODE,
    G_PARAM_SYM,
    G_PARAM_EDGE,
    G_WILDCARD_EDGE,
    G_TRAILING_EDGE,
    G_LITERAL_SYM,
    G_LITERAL_EDGE,
};

void print_graph(wrouter_t *router)
{
    enum graph_entry m = G_NODE;
    uint16_t node = 0, *cursor = router->graph;
    uint16_t n = 0;

    for (uint16_t i = 0; i < router->graph_size; i++) {
        cursor = router->graph + i;

        fprintf(stderr, "%04x: %04x ", i, router->graph[i]);

        switch (m)
        {
            case G_NODE:
                node = *cursor;
                n = node & NODE_LITERALS_MASK;

                fprintf(stderr, "NODE");
                if (i == 0)
                    fprintf(stderr, " ROOT");
                if (node & NODE_FLAG_TERMINAL)
                    fprintf(stderr, " TERMINAL");
                fprintf(stderr, "\n");

                if (node & NODE_FLAG_HAS_PARAM) {
                    m = G_PARAM_SYM;
                    continue;
                }
                if (node & NODE_FLAG_HAS_WILDCARD) {
                    m = G_WILDCARD_EDGE;
                    continue;
                }
                if (node & NODE_FLAG_HAS_TRAILING) {
                    m = G_TRAILING_EDGE;
                    continue;
                }
                if (n) {
                    m = G_LITERAL_SYM;
                    continue;
                }
                break;

            case G_PARAM_SYM:
                m = G_PARAM_EDGE;
                fprintf(stderr, "  SYMBOL PARAMETER --> %s\n", symbol_lookup(&router->params, *cursor));
                continue;

            case G_PARAM_EDGE:
                fprintf(stderr, "  EDGE PARAMETER\n");
                if (node & NODE_FLAG_HAS_TRAILING) {
                    m = G_TRAILING_EDGE;
                    continue;
                }
                if (n) {
                    m = G_LITERAL_SYM;
                    continue;
                }
                break;

            case G_WILDCARD_EDGE:
                fprintf(stderr, "  EDGE WILDCARD\n");
                if (node & NODE_FLAG_HAS_TRAILING) {
                    m = G_TRAILING_EDGE;
                    continue;
                }
                if (n) {
                    m = G_LITERAL_SYM;
                    continue;
                }
                break;

            case G_TRAILING_EDGE:
                fprintf(stderr, "  EDGE TRAILING\n");
                if (n) {
                    m = G_LITERAL_SYM;
                    continue;
                }
                break;

            case G_LITERAL_SYM:
                m = G_LITERAL_EDGE;
                fprintf(stderr, "  SYMBOL LITERAL --> %s\n", symbol_lookup(&router->literals, *cursor));
                continue;

            case G_LITERAL_EDGE:
                fprintf(stderr, "  EDGE LITERAL\n");
                if (--n) {
                    m = G_LITERAL_SYM;
                    continue;
                }
                m = G_NODE;
                break;
        }
        m = G_NODE;
    }
}

static void print_route_node(const segment_t *seg, int depth, int is_param)
{
    // Indent.
    for (int i = 0; i < depth; i++)
        printf("  ");

    if (is_param)
        printf(":");

    // Current node.
    if (seg->str != NULL)
        printf("%.*s", seg->str_length, seg->str);
    else
        printf("/");

    if (seg->terminal != NULL)
        printf(" &");

    printf("\n");

    // Trailing-slash.
    if (seg->trailing != NULL) {
        for (int i = 0; i < depth + 1; i++)
            printf("  ");

        printf("/ &\n");
    }

    // Literal children.
    for (const segment_t *child = seg->head; child; child = child->next)
        print_route_node(child, depth + 1, 0);

    // Param child.
    if (seg->spec_type == SPEC_PARAM && seg->special.param != NULL)
        print_route_node(seg->special.param, depth + 1, 1);

    // Wildcard route.
    if (seg->spec_type == SPEC_WILDCARD && seg->special.wildcard != NULL) {
        for (int i = 0; i < depth + 1; i++)
            printf("  ");

        printf("* &\n");
    }
}

void builder_print_tree(const wrouter_builder_t *builder)
{
    if (builder == NULL || builder->root == NULL) {
        printf("(empty)\n");
        return;
    }

    print_route_node(builder->root, 0, 0);
}
