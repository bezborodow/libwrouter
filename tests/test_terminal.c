#include "terminal.h"
#include "wrouter.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>


void test_terminal_lookup(void)
{
    uint16_t route_ctx1 = 1000, route_ctx2 = 2000;

    wrouter_route_t *terminal = NULL;
    wrouter_route_t terminal_routes[] = {
        {
            .handler = NULL,
            .ctx = &route_ctx1,
        },
        {
            .handler = NULL,
            .ctx = &route_ctx2,
        }
    };

    uint16_t refs[] = { 4, 8 };

    // The terminal lookup is designed so that terminals can be found by their
    // graph offset reference. When the graph is built, all terminal nodes
    // should have an entry in refs corresponding to their graph offset. The
    // index of the ref is used to retrieve the terminal route handler and
    // route context.
    terminals_t terminals = { 0 };
    terminals.refs = refs;
    terminals.base = terminal_routes;
    terminals.count = 2; 
    
    // Lookup first terminal by its ref of 4.
    terminal = terminal_lookup(&terminals, 4);
    assert(terminal != NULL);
    assert(terminal->ctx != NULL);
    assert(*(uint16_t *)terminal->ctx == 1000);

    // Lookup second terminal by its ref of 8.
    terminal = terminal_lookup(&terminals, 8);
    assert(terminal != NULL);
    assert(terminal->ctx != NULL);
    assert(*(uint16_t *)terminal->ctx == 2000);

    // Lookup a missing termianl. This shouldn't ever happen if the graph is
    // built correctly!
    terminal = terminal_lookup(&terminals, 16);
    assert(terminal == NULL);
}

int main(void)
{
    test_terminal_lookup();

    return 0;
}
