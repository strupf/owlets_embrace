// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef SYS_TYPES_H
#define SYS_TYPES_H

#define SYS_CONFIG_EDIT_PD      0 // allow editing of Playdate specific stuff inside VS
#define SYS_CONFIG_ONLY_BACKEND 0 // only barebones engine stuff
#define SYS_CONFIG_DEBUG        1

#if SYS_CONFIG_DEBUG
#define SYS_DEBUG
#endif

#if SYS_CONFIG_EDIT_PD
#undef SYS_SDL
#define SYS_PD
#define SYS_PD_HW
#endif

#if !defined(SYS_SDL) && !defined(SYS_PD)
#error "sys_types.h: No platform defined"
#endif

#if defined(SYS_SDL) && defined(SYS_PD)
#error "sys_types.h: More than one platform defined"
#endif

#include <math.h>
#include <stdalign.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef SYS_PD_HW // assertions don't work on hardware - disable
#undef assert
#undef static_assert
#define assert(X)
#define static_assert(A, B)
#elif defined(SYS_SDL) // use SDL assert for desktop
#include "SDL2/SDL_assert.h"
#undef assert
#define assert SDL_assert
#else
#include <assert.h>
#endif

#if defined(SYS_SDL)
#include <stdio.h>
#define sys_printf(...)                                    \
    {                                                      \
        char strret[1024] = {0};                           \
        snprintf(strret, sizeof(strret) - 1, __VA_ARGS__); \
        sys_log(strret);                                   \
        printf("%s", strret);                              \
    }
#else
#ifdef SYS_PD
#ifndef TARGET_EXTENSION
#define TARGET_EXTENSION 1
#elif (defined(TARGET_EXTENSION) && (TARGET_EXTENSION != 1))
#undef TARGET_EXTENSION
#define TARGET_EXTENSION 1
#endif
#endif

#include "PD/pd_api.h"
PlaydateAPI *PD;

extern int (*PD_format_str)(char **ret, const char *format, ...);
extern void *(*PD_realloc)(void *ptr, size_t size);
void sys_log(const char *str);

#ifdef SYS_PD_HW
#define sys_printf(...)                      \
    {                                        \
        char *strret;                        \
        PD_format_str(&strret, __VA_ARGS__); \
        sys_log(strret);                     \
        PD_realloc(strret, 0);               \
    }
#else
extern void (*PD_log)(const char *format, ...);
#define sys_printf(...)                      \
    {                                        \
        char *strret;                        \
        PD_format_str(&strret, __VA_ARGS__); \
        sys_log(strret);                     \
        PD_log(__VA_ARGS__);                 \
        PD_realloc(strret, 0);               \
    }
#endif
#endif

typedef unsigned char  uchar;
typedef unsigned short ushort;
typedef unsigned int   uint;
typedef uint8_t        u8;
typedef uint16_t       u16;
typedef uint32_t       u32;
typedef uint64_t       u64;
typedef int8_t         i8;
typedef int16_t        i16;
typedef int32_t        i32;
typedef int64_t        i64;
typedef i8             bool8;
typedef i16            bool16;
typedef i32            bool32;
typedef u8             flags8;
typedef u16            flags16;
typedef u32            flags32;
typedef u64            flags64;
typedef float          f32;
typedef double         f64;
typedef size_t         usize;
typedef uintptr_t      uptr;
typedef intptr_t       iptr;
typedef u32            u16x2;
typedef i32            i16x2;
typedef u32            u8x4;
typedef i32            i8x4;

// used for user defined allocations
// alloc(ctx, size) -> ctx: pointer to some memory manager
typedef struct {
    void *(*allocf)(void *ctx, usize s);
    void *ctx;
} alloc_s;

#define I64_MAX INT64_MAX
#define I64_MIN INT64_MIN
#define U64_MAX UINT64_MAX
#define U64_MIN 0
#define I32_MAX INT32_MAX
#define I32_MIN INT32_MIN
#define U32_MAX UINT32_MAX
#define U32_MIN 0
#define I16_MAX INT16_MAX
#define I16_MIN INT16_MIN
#define U16_MAX UINT16_MAX
#define U16_MIN 0
#define I8_MAX  INT8_MAX
#define I8_MIN  INT8_MIN
#define U8_MAX  UINT8_MAX
#define U8_MIN  0

#define POW2(X)          ((X) * (X))
#define GLUE2(A, B)      A##B
#define GLUE(A, B)       GLUE2(A, B)
#define ARRLEN(A)        (sizeof(A) / sizeof(A[0]))
#define MAX(A, B)        ((A) >= (B) ? (A) : (B))
#define MIN(A, B)        ((A) <= (B) ? (A) : (B))
#define ABS(A)           ((A) >= 0 ? (A) : -(A))
#define SGN(A)           ((0 < (A)) - (0 > (A)))
#define CLAMP(X, LO, HI) ((X) > (HI) ? (HI) : ((X) < (LO) ? (LO) : (X)))
#define SWAP(T, a, b)  \
    do {               \
        T tmp_ = a;    \
        a      = b;    \
        b      = tmp_; \
    } while (0)

#ifndef SYS_PD_HW
#define FILE_AND_LINE__(A, B) A "|" #B
#define FILE_AND_LINE_(A, B)  FILE_AND_LINE__(A, B)
#define FILE_AND_LINE         FILE_AND_LINE_(__FILE__, __LINE__)
#else
#define FILE_AND_LINE ""
#endif

#define NOT_IMPLEMENTED                        \
    do {                                       \
        sys_printf("+++ NOT IMPLEMENTED +++"); \
        sys_printf(FILE_AND_LINE);             \
        assert(0);                             \
    } while (0);
#define BAD_PATH                        \
    do {                                \
        sys_printf("+++ BAD PATH +++"); \
        sys_printf(FILE_AND_LINE);      \
        assert(0);                      \
    } while (0);

typedef struct {
    i32 x, y, w, h;
} rec_i32;

typedef struct {
    i8 x, y;
} v2_i8;

typedef struct {
    i16 x, y;
} v2_i16;

typedef struct {
    i32 x, y;
} v2_i32;

static inline v2_i32 v2_i32_from_i16(v2_i16 a)
{
    v2_i32 r = {a.x, a.y};
    return r;
}

typedef struct {
    f32 x, y;
} v2_f32;

typedef struct {
    v2_i32 p[3];
} tri_i32;

typedef struct {
    v2_i32 p;
    i32    r;
} cir_i32;

typedef struct { // A-----B    segment
    v2_i32 a;
    v2_i32 b;
} lineseg_i32;

typedef struct { // A-----B--- ray
    v2_i32 a;    //
    v2_i32 b;    // inf
} lineray_i32;

typedef struct { // ---A-----B--- line
    v2_i32 a;    // inf
    v2_i32 b;    // inf
} line_i32;

typedef struct {
    f32 m[9];
} m33_f32;

typedef v2_i32   v2i;
typedef v2_f32   v2f;
typedef rec_i32  recti;
typedef tri_i32  trianglei;
typedef line_i32 linei;

// int (*cmp)(const void *a, const void *b)
// -: a goes before b | b goes after a
// 0: equivalent
// +: a goes after b | b goes before a
typedef int (*cmp_f)(const void *a, const void *b);
static void _quicksort(void *base, int lo, int hi, usize s, cmp_f cmp);

static void sort_array(void *base, int num, usize s, cmp_f cmp)
{
    if (num <= 1) return;
    assert(base && s && cmp);
    _quicksort(base, 0, num - 1, s, cmp);
}

static void _quicksort(void *base, int lo, int hi, usize s, cmp_f cmp)
{
    static char m[1024];
    assert(s <= sizeof(m));
    assert(lo < hi);

    int   i = lo;
    int   j = hi;
    char *a = (char *)base + i * s;
    char *b = (char *)base + j * s;
    char *p = (char *)base + ((lo + hi) >> 1) * s;
    while (i < j) {
        while (cmp((const void *)a, (const void *)p) < 0) {
            a += s, i++;
        }
        while (cmp((const void *)b, (const void *)p) > 0) {
            b -= s, j--;
        }

        if (i != j) {
            memcpy(m, (const void *)a, s);
            memcpy(a, (const void *)b, s);
            memcpy(b, (const void *)m, s);
        }

        a += s, i++;
        b -= s, j--;
    } // -> moved to end because we know that lo/i is < hi/j

    if (lo < j) _quicksort(base, lo, j, s, cmp);
    if (i < hi) _quicksort(base, i, hi, s, cmp);
}

// =============================================================================
// B8(00001111) -> 0x0F
// B16(10000000, 00001111) -> 0x800F
#define _B_(A)          GLUE(_B, A)
#define B4(A)           GLUE(0x, _B_(GLUE(0000, A)))
#define B8(A)           GLUE(0x, _B_(A))
#define B16(A, B)       GLUE(0x, GLUE(_B_(A), _B_(B)))
#define B24(A, B, C)    GLUE(0x, GLUE(GLUE(_B_(A), _B_(B)), _B_(C)))
#define B32(A, B, C, D) GLUE(GLUE(0x, GLUE(GLUE(_B_(A), _B_(B)), GLUE(_B_(C), _B_(D)))), U)

#define _B00000000 00
#define _B00000001 01
#define _B00000010 02
#define _B00000011 03
#define _B00000100 04
#define _B00000101 05
#define _B00000110 06
#define _B00000111 07
#define _B00001000 08
#define _B00001001 09
#define _B00001010 0A
#define _B00001011 0B
#define _B00001100 0C
#define _B00001101 0D
#define _B00001110 0E
#define _B00001111 0F
#define _B00010000 10
#define _B00010001 11
#define _B00010010 12
#define _B00010011 13
#define _B00010100 14
#define _B00010101 15
#define _B00010110 16
#define _B00010111 17
#define _B00011000 18
#define _B00011001 19
#define _B00011010 1A
#define _B00011011 1B
#define _B00011100 1C
#define _B00011101 1D
#define _B00011110 1E
#define _B00011111 1F
#define _B00100000 20
#define _B00100001 21
#define _B00100010 22
#define _B00100011 23
#define _B00100100 24
#define _B00100101 25
#define _B00100110 26
#define _B00100111 27
#define _B00101000 28
#define _B00101001 29
#define _B00101010 2A
#define _B00101011 2B
#define _B00101100 2C
#define _B00101101 2D
#define _B00101110 2E
#define _B00101111 2F
#define _B00110000 30
#define _B00110001 31
#define _B00110010 32
#define _B00110011 33
#define _B00110100 34
#define _B00110101 35
#define _B00110110 36
#define _B00110111 37
#define _B00111000 38
#define _B00111001 39
#define _B00111010 3A
#define _B00111011 3B
#define _B00111100 3C
#define _B00111101 3D
#define _B00111110 3E
#define _B00111111 3F
#define _B01000000 40
#define _B01000001 41
#define _B01000010 42
#define _B01000011 43
#define _B01000100 44
#define _B01000101 45
#define _B01000110 46
#define _B01000111 47
#define _B01001000 48
#define _B01001001 49
#define _B01001010 4A
#define _B01001011 4B
#define _B01001100 4C
#define _B01001101 4D
#define _B01001110 4E
#define _B01001111 4F
#define _B01010000 50
#define _B01010001 51
#define _B01010010 52
#define _B01010011 53
#define _B01010100 54
#define _B01010101 55
#define _B01010110 56
#define _B01010111 57
#define _B01011000 58
#define _B01011001 59
#define _B01011010 5A
#define _B01011011 5B
#define _B01011100 5C
#define _B01011101 5D
#define _B01011110 5E
#define _B01011111 5F
#define _B01100000 60
#define _B01100001 61
#define _B01100010 62
#define _B01100011 63
#define _B01100100 64
#define _B01100101 65
#define _B01100110 66
#define _B01100111 67
#define _B01101000 68
#define _B01101001 69
#define _B01101010 6A
#define _B01101011 6B
#define _B01101100 6C
#define _B01101101 6D
#define _B01101110 6E
#define _B01101111 6F
#define _B01110000 70
#define _B01110001 71
#define _B01110010 72
#define _B01110011 73
#define _B01110100 74
#define _B01110101 75
#define _B01110110 76
#define _B01110111 77
#define _B01111000 78
#define _B01111001 79
#define _B01111010 7A
#define _B01111011 7B
#define _B01111100 7C
#define _B01111101 7D
#define _B01111110 7E
#define _B01111111 7F
#define _B10000000 80
#define _B10000001 81
#define _B10000010 82
#define _B10000011 83
#define _B10000100 84
#define _B10000101 85
#define _B10000110 86
#define _B10000111 87
#define _B10001000 88
#define _B10001001 89
#define _B10001010 8A
#define _B10001011 8B
#define _B10001100 8C
#define _B10001101 8D
#define _B10001110 8E
#define _B10001111 8F
#define _B10010000 90
#define _B10010001 91
#define _B10010010 92
#define _B10010011 93
#define _B10010100 94
#define _B10010101 95
#define _B10010110 96
#define _B10010111 97
#define _B10011000 98
#define _B10011001 99
#define _B10011010 9A
#define _B10011011 9B
#define _B10011100 9C
#define _B10011101 9D
#define _B10011110 9E
#define _B10011111 9F
#define _B10100000 A0
#define _B10100001 A1
#define _B10100010 A2
#define _B10100011 A3
#define _B10100100 A4
#define _B10100101 A5
#define _B10100110 A6
#define _B10100111 A7
#define _B10101000 A8
#define _B10101001 A9
#define _B10101010 AA
#define _B10101011 AB
#define _B10101100 AC
#define _B10101101 AD
#define _B10101110 AE
#define _B10101111 AF
#define _B10110000 B0
#define _B10110001 B1
#define _B10110010 B2
#define _B10110011 B3
#define _B10110100 B4
#define _B10110101 B5
#define _B10110110 B6
#define _B10110111 B7
#define _B10111000 B8
#define _B10111001 B9
#define _B10111010 BA
#define _B10111011 BB
#define _B10111100 BC
#define _B10111101 BD
#define _B10111110 BE
#define _B10111111 BF
#define _B11000000 C0
#define _B11000001 C1
#define _B11000010 C2
#define _B11000011 C3
#define _B11000100 C4
#define _B11000101 C5
#define _B11000110 C6
#define _B11000111 C7
#define _B11001000 C8
#define _B11001001 C9
#define _B11001010 CA
#define _B11001011 CB
#define _B11001100 CC
#define _B11001101 CD
#define _B11001110 CE
#define _B11001111 CF
#define _B11010000 D0
#define _B11010001 D1
#define _B11010010 D2
#define _B11010011 D3
#define _B11010100 D4
#define _B11010101 D5
#define _B11010110 D6
#define _B11010111 D7
#define _B11011000 D8
#define _B11011001 D9
#define _B11011010 DA
#define _B11011011 DB
#define _B11011100 DC
#define _B11011101 DD
#define _B11011110 DE
#define _B11011111 DF
#define _B11100000 E0
#define _B11100001 E1
#define _B11100010 E2
#define _B11100011 E3
#define _B11100100 E4
#define _B11100101 E5
#define _B11100110 E6
#define _B11100111 E7
#define _B11101000 E8
#define _B11101001 E9
#define _B11101010 EA
#define _B11101011 EB
#define _B11101100 EC
#define _B11101101 ED
#define _B11101110 EE
#define _B11101111 EF
#define _B11110000 F0
#define _B11110001 F1
#define _B11110010 F2
#define _B11110011 F3
#define _B11110100 F4
#define _B11110101 F5
#define _B11110110 F6
#define _B11110111 F7
#define _B11111000 F8
#define _B11111001 F9
#define _B11111010 FA
#define _B11111011 FB
#define _B11111100 FC
#define _B11111101 FD
#define _B11111110 FE
#define _B11111111 FF

#endif