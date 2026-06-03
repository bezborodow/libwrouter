#include "graph.h"
#include <assert.h>
#include <string.h>
#include <stdbool.h>

void test_graph_append_empty()
{
    size_t cursor = 0;
    graph_append(NULL, &cursor, 0, _Alignof(node_t));
}

int main(void)
{
    test_graph_append_empty();

    return 0;
}
