enum graph_entry {
    G_NODE,
    G_PARAM,
    G_WILDCARD,
    G_TRAILING,
    G_LITERAL,
};

void router_print_graph(const wrouter_t *router)
{
    const void *g = router->graph;
    if (g == NULL)
        return;

    size_t i_inf = 0;
    uint16_t n_literals = 0;
    uint16_t i_lit = 0;

    const node_t *cur;
    const edge_t *edge = NULL;

    uintptr_t base = (uintptr_t)g;
    uintptr_t end = base + router->graph_size;
    uintptr_t next = (uintptr_t)g;

    enum graph_entry entry = G_NODE;

    while (next < end) {

        if (entry == G_NODE) {
            printf("---------------------------------\n");
            cur = (node_t *)next;
        }

        printf("%04lx:  ", next - base);

        next += entry == G_NODE ? sizeof(node_t) : sizeof(edge_t);

        switch (entry) {
            case G_NODE:
                printf("N");
                edge = node_edge_base(cur);
                n_literals = cur->data & NODE_LITERALS_MASK;
                i_lit = 0;

                if (cur->data & NODE_FLAG_HAS_PARAM)
                    entry = G_PARAM;
                else if (cur->data & NODE_FLAG_HAS_WILDCARD)
                    entry = G_WILDCARD;
                else if (cur->data & NODE_FLAG_HAS_TRAILING)
                    entry = G_TRAILING;
                else if (n_literals) {
                    entry = G_LITERAL;
                }

                if (cur->data & NODE_FLAG_TERMINAL)
                    printf(" &");

                // printf("        (found %u literals)\n", n_literals);
                printf("\n");

                break;

            case G_PARAM:
                printf("EP :%s\n", symbol_lookup(&router->params, edge->symbol));

                if (cur->data & NODE_FLAG_HAS_TRAILING)
                    entry = G_TRAILING;
                else if (n_literals) {
                    entry = G_LITERAL;
                } else {
                    entry = G_NODE;
                }

                edge++;
                break;

            case G_WILDCARD:
                printf("EW *\n");

                if (cur->data & NODE_FLAG_HAS_TRAILING)
                    entry = G_TRAILING;
                else if (n_literals) {
                    entry = G_LITERAL;
                } else {
                    entry = G_NODE;
                }
                edge++;
                break;

            case G_TRAILING:
                printf("ET /\n");
                if (n_literals) {
                    entry = G_LITERAL;
                } else {
                    entry = G_NODE;
                }
                edge++;
                break;

            case G_LITERAL:
                printf("EL '%s' (%u/%u) ", symbol_lookup(&router->literals, edge->symbol),
                       i_lit + 1, n_literals);
                printf("--> %04lx\n", edge->next << 1);

                if (++i_lit < n_literals)
                    edge++;
                else
                    entry = G_NODE;
                break;
        }

        // Catch infinite loops.
        if (i_inf++ > 2000 * sizeof(edge_t) * 4) {
            break;
        }
        if (i_inf++ > GRAPH_CAPACITY_BYTES * sizeof(edge_t) * 4) {
            assert(0);
        }
    }
}
