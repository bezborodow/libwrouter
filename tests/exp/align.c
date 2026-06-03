#include <stdio.h>
#include <stdint.h>

typedef struct {
    uint16_t data;
} node_t;

typedef struct {
    uint16_t symbol;
    uint16_t next;
} edge_t;

typedef struct {
    uint32_t data;
    uint8_t padding;
} pad_t;

typedef struct {
    uint8_t padding0;
    uint8_t padding1;
    uint8_t padding2;
    uint8_t padding3;
    uint8_t padding4;
    uint8_t padding5;
    uint8_t padding6;
} paad_t;

typedef struct {
    uint32_t uint;
} s32_t;

typedef struct {
    uint64_t uint;
} s64_t;

typedef struct {
    uint32_t real;
    uint32_t imag;
} c32_t;

typedef struct {
    uint64_t real;
    uint64_t imag;
} c64_t;

int main(void)
{
    printf("Node: %lu (%lu)\n", sizeof(node_t), _Alignof(node_t));
    printf("Edge: %lu (%lu)\n", sizeof(edge_t), _Alignof(edge_t));
    printf("Pad : %lu (%lu)\n", sizeof(pad_t), _Alignof(pad_t));
    printf("Paad: %lu (%lu)\n", sizeof(paad_t), _Alignof(paad_t));
    printf("ui8 : %lu (%lu)\n", sizeof(uint8_t), _Alignof(uint8_t));
    printf("ui16: %lu (%lu)\n", sizeof(uint16_t), _Alignof(uint16_t));
    printf("ui32: %lu (%lu)\n", sizeof(uint32_t), _Alignof(uint32_t));
    printf("ui64: %lu (%lu)\n", sizeof(uint64_t), _Alignof(uint64_t));
    printf("s32 : %lu (%lu)\n", sizeof(s32_t), _Alignof(s32_t));
    printf("s64 : %lu (%lu)\n", sizeof(s64_t), _Alignof(s64_t));
    printf("c32 : %lu (%lu)\n", sizeof(c32_t), _Alignof(c32_t));
    printf("c64 : %lu (%lu)\n", sizeof(c64_t), _Alignof(c64_t));

    return 0;
}
