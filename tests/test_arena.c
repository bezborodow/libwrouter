#include "arena.h"
#include <assert.h>
#include <string.h>

static void test_arena_basic_alloc(void)
{
    arena_t a = { 0 };

    char *p = arena_alloc(&a, 16);

    assert(p != NULL);
    memset(p, 0xAA, 16);
}

static void test_arena_multiple_allocs(void)
{
    arena_t a = { 0 };

    char *p1 = arena_alloc(&a, 16);
    char *p2 = arena_alloc(&a, 16);

    assert(p1 != NULL);
    assert(p2 != NULL);
    assert(p1 != p2);

    memset(p1, 0x11, 16);
    memset(p2, 0x22, 16);
}

static void test_arena_stability_after_growth(void)
{
    arena_t a = { 0 };

    char *p1 = arena_alloc(&a, 4000);
    char *p2 = arena_alloc(&a, 4000);

    assert(p1 != NULL);
    assert(p2 != NULL);
    assert(p1 != p2);

    // ensure both still usable
    p1[0] = 'a';
    p2[0] = 'b';

    assert(p1[0] == 'a');
    assert(p2[0] == 'b');
}

static void test_arena_free(void)
{
    arena_t a = { 0 };

    arena_alloc(&a, 128);
    arena_free(&a);

    assert(a.head == NULL);
}

int main(void)
{
    test_arena_basic_alloc();
    test_arena_multiple_allocs();
    test_arena_stability_after_growth();
    test_arena_free();

    return 0;
}
