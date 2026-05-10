#include "arena.h"
#include <assert.h>
#include <string.h>

static void test_arena_basic_alloc(void)
{
    arena_t a = { 0 };

    void *p = arena_alloc(&a, 16);

    assert(p != NULL);
    assert(a.base != NULL);
    assert(a.size >= 16);
    assert(a.used == 16);
}

static void test_arena_growth(void)
{
    arena_t a = { 0 };

    void *p1 = arena_alloc(&a, 1024);
    void *p2 = arena_alloc(&a, 2048);

    assert(p1 != NULL);
    assert(p2 != NULL);
    assert(a.used == 3072);
    assert(a.size >= 3072);
}

static void test_arena_contiguity(void)
{
    arena_t a = { 0 };

    void *p1 = arena_alloc(&a, 8);
    void *p2 = arena_alloc(&a, 8);

    assert(p1 != NULL);
    assert(p2 != NULL);
    assert((char *)p2 == (char *)p1 + 8);
}

static void test_arena_free(void)
{
    arena_t a = { 0 };

    arena_alloc(&a, 128);
    arena_free(&a);

    assert(a.base == NULL);
    assert(a.size == 0);
    assert(a.used == 0);
}

int main(void)
{
    test_arena_basic_alloc();
    test_arena_growth();
    test_arena_contiguity();
    test_arena_free();

    return 0;
}
