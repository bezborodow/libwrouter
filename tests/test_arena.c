#include "arena.h"
#include <assert.h>
#include <string.h>
#include <stdint.h>

typedef struct {
    double a;
    double b;
    double c;
    char d;
} test_t;

static void test_arena_basic_alloc(void)
{
    arena_t a = { 0 };

    char *p = arena_alloc(&a, 16);

    assert(p != NULL);
    memset(p, 0xAA, 16);
    assert(arena_used(&a) == 16);

    arena_free(&a);
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

    assert(arena_used(&a) == 32);

    arena_free(&a);
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

    assert(arena_used(&a) == 8000);

    arena_free(&a);
}

static void test_arena_free(void)
{
    arena_t a = { 0 };

    arena_alloc(&a, 128);
    arena_free(&a);

    assert(a.head == NULL);
}

static void test_arena_alignment_struct(void)
{
    arena_t a = { 0 };

    test_t *p1 = arena_alloc_aligned(&a, sizeof(test_t), _Alignof(test_t));
    test_t *p2 = arena_alloc_aligned(&a, sizeof(test_t), _Alignof(test_t));

    assert(p1 != NULL);
    assert(p2 != NULL);

    assert(((uintptr_t)p1 % _Alignof(test_t)) == 0);
    assert(((uintptr_t)p2 % _Alignof(test_t)) == 0);

    arena_free(&a);
}

static void test_arena_mixed_alignment(void)
{
    arena_t a = { 0 };

    char *c = arena_alloc_aligned(&a, 1, _Alignof(char));
    int *i = arena_alloc_aligned(&a, sizeof(int), _Alignof(int));
    double *d = arena_alloc_aligned(&a, sizeof(double), _Alignof(double));

    assert(((uintptr_t)c % _Alignof(char)) == 0);
    assert(((uintptr_t)i % _Alignof(int)) == 0);
    assert(((uintptr_t)d % _Alignof(double)) == 0);

    arena_free(&a);
}

static void test_arena_alignment_stress(void)
{
    arena_t a = { 0 };

    for (int i = 0; i < 1000; i++) {
        size_t align = (i % 64) + 1;
        if ((align & (align - 1)) != 0)
            align = 1u << (31 - __builtin_clz(align)); // make power of two

        void *p = arena_alloc_aligned(&a, 17, align);
        assert(((uintptr_t)p % align) == 0);
    }

    arena_free(&a);
}

int main(void)
{
    test_arena_basic_alloc();
    test_arena_multiple_allocs();
    test_arena_stability_after_growth();
    test_arena_free();
    test_arena_alignment_struct();
    test_arena_mixed_alignment();
    test_arena_alignment_stress();

    return 0;
}
