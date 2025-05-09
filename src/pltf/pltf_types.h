// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef PLTF_TYPES_H
#define PLTF_TYPES_H

#define PLTF_DEBUG      0
#define PLTF_ENABLE_LOG 0
#define PLTF_EDIT_PD    0 // edit PD code files? -> enable PLTF_PD_HW code paths

#if PLTF_EDIT_PD
#undef PLTF_SDL
#define PLTF_PD    1
#define PLTF_PD_HW 1
#endif

#if !PLTF_PD && !PLTF_SDL
#error NO PLATFORM DEFINED!
#endif
#if PLTF_PD && PLTF_SDL
#error TWO PLATFORMS DEFINED!
#endif

#ifdef __EMSCRIPTEN__
#define PLTF_SDL_WEB 1
#include <emscripten.h>
// void emscripten_set_main_loop();
#else
#define PLTF_SDL_WEB 0
#endif

#include <assert.h>
#include <ctype.h>
#include <float.h>
#include <math.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#if PLTF_PD_HW
#define PREFETCH __builtin_prefetch
#else
#define PREFETCH(X)
#endif

#if PLTF_PD_HW
void (*PD_system_error)(const char *format, ...);
#if 1
#undef assert
#define assert(X)
#else
#undef assert
#define assert(X)                                \
    {                                            \
        if (!(X)) {                              \
            PD_system_error("ASSERT: " #X "\n"); \
        }                                        \
    }
#endif
#elif PLTF_SDL && 1 // use SDL assert instead of std?
#include "SDL2/SDL_assert.h"
#undef assert
#define assert SDL_assert
#endif

#if !PLTF_DEBUG
#undef assert
#define assert(X)
#endif

typedef char           byte;
typedef float          f32;
typedef double         f64;
typedef uintptr_t      uptr;
typedef intptr_t       iptr;
typedef unsigned char  uchar;
typedef unsigned short ushort;
typedef unsigned int   uint;
typedef unsigned long  ulong;
typedef size_t         usize;
typedef uint8_t        u8;
typedef uint16_t       u16;
typedef uint32_t       u32;
typedef uint64_t       u64;
typedef int8_t         i8;
typedef int16_t        i16;
typedef int32_t        i32;
typedef int64_t        i64;
typedef u8             bool8;
typedef u16            bool16;
typedef u32            bool32;
typedef bool8          b8;
typedef bool16         b16;
typedef bool32         b32;
typedef i32            err32;

#define I64_MAX  INT64_MAX
#define I64_MIN  INT64_MIN
#define U64_MAX  UINT64_MAX
#define U64_MIN  0
#define I32_MAX  INT32_MAX
#define I32_MIN  INT32_MIN
#define U32_MAX  UINT32_MAX
#define U32_MIN  0
#define I16_MAX  INT16_MAX
#define I16_MIN  INT16_MIN
#define U16_MAX  UINT16_MAX
#define U16_MIN  0
#define I8_MAX   INT8_MAX
#define I8_MIN   INT8_MIN
#define U8_MAX   UINT8_MAX
#define U8_MIN   0
#define U32_C(X) (X##U)
#define I64_C(X) (X##LL)
#define U64_C(X) (X##ULL)

#define Q_X(X, ONE) (i32)((f32)(X) * (ONE))
#define Q_8(X)      Q_X(X, 256.f)
#define Q_10(X)     Q_X(X, 1024.f)
#define Q_12(X)     Q_X(X, 4096.f)
#define Q_15(X)     Q_X(X, 32768.f)
#define Q_16(X)     Q_X(X, 65536.f)

#if __cplusplus // C++
#define CINIT(T) T
#if (__cplusplus < 201103L) // earlier than C++11
#error CPP VERSION NOT SUPPORTED
#endif
#else // C
#define CINIT(T) (T)
#if (__STDC_VERSION__ < 201112L) // earlier than C11
#error C VERSION NOT SUPPORTED
#elif (__STDC_VERSION__ < 202311L) // earler than C23
#include <stdalign.h>
#endif
#endif

#define ALIGNAS     alignas
#define ALIGNOF     alignof
#define ALIGNAST(T) ALIGNAS(ALIGNOF(T))

static inline void *align_ptr(void *p, usize alignment)
{
    return (void *)(((uptr)p + alignment - 1) & ~(alignment - 1));
}

static inline usize align_usize(usize s, usize alignment)
{
    return (s + alignment - 1) & ~(alignment - 1);
}

// linux style circular, double linked list with a dummy node as a header
typedef struct lnode_s lnode_s;
struct lnode_s {
    lnode_s *next;
    lnode_s *prev;
};

#define LNODE_GET(PTR, T, MEMBER) (T *)((uptr)(PTR) - offsetof(T, MEMBER))
#define LNODE_FOR_EACH(PHEAD, IT) \
    for (lnode_s *IT = (PHEAD)->next; IT != (PHEAD); IT = IT->next)

static inline void lnode_head_init(lnode_s *n)
{
    n->next = n;
    n->prev = n;
}

#define lnode_add lnode_add_after

static bool32 lnode_empty(lnode_s *n)
{
    return (n->next == n);
}

// adds a node after n
static inline void lnode_add_after(lnode_s *n, lnode_s *n_to_add)
{
    n->next->prev  = n_to_add;
    n_to_add->next = n->next;
    n_to_add->prev = n;
    n->next        = n_to_add;
}

// adds a node before n
static inline void lnode_add_before(lnode_s *n, lnode_s *n_to_add)
{
    n->prev->next  = n_to_add;
    n_to_add->prev = n->prev;
    n_to_add->next = n;
    n->prev        = n_to_add;
}

static inline void lnode_del(lnode_s *n)
{
    n->next->prev = n->prev;
    n->prev->next = n->next;
}

// used for user defined allocations
// alloc(ctx, size) -> ctx: pointer to some memory manager
typedef struct {
    void *(*allocf)(void *ctx, usize s);
    void *ctx;
} alloc_s;

typedef struct {
    void *(*allocfunc)(void *ctx, usize s, usize alignment);
    void *ctx;
} allocator_s;

// clang-format off
#define typedef_struct(NAME)  typedef struct NAME NAME; struct NAME
#define typedef_union(NAME)   typedef union NAME NAME; union NAME
#define mset                  memset
#define mcpy                  memcpy
#define mmov                  memmove
#define mclr(DST, SIZE)       mset(DST, 0, SIZE)
#define mclr_static_arr(DST)  mset(DST, 0, sizeof(DST))
#define mclr_field(DST)       mset(&(DST), 0, sizeof(DST))
#define POW2(X)               ((X) * (X))
#define GLUE2(A, B)           A##B
#define GLUE(A, B)            GLUE2(A, B)
#define ARRLEN(A)             (i32)(sizeof(A) / sizeof(A[0]))
#define MAX(A, B)             ((A) >= (B) ? (A) : (B))
#define MIN(A, B)             ((A) <= (B) ? (A) : (B))
#define ABS(A)                ((A) >= 0 ? (A) : -(A))
#define SGN(A)                ((0 < (A)) - (0 > (A)))
#define CLAMP(X, LO, HI)      ((X) > (HI) ? (HI) : ((X) < (LO) ? (LO) : (X)))
#define SWAP(T, a, b)         { T tmp_ = a; a = b; b = tmp_; }
#define MKILOBYTE(X) ((X) * 1024)
#define MMEGABYTE(X) ((X) * 1024 * 1024)
#define FILE_AND_LINE__(A, B) A "|" #B
#define FILE_AND_LINE_(A, B)  FILE_AND_LINE__(A, B)
#define FILE_AND_LINE         FILE_AND_LINE_(__FILE__, __LINE__)
// clang-format on

#define NOT_IMPLEMENTED                      \
    do {                                     \
        pltf_log("+++ NOT IMPLEMENTED +++"); \
        pltf_log(FILE_AND_LINE);             \
        assert(0);                           \
    } while (0);
#define BAD_PATH                      \
    do {                              \
        pltf_log("+++ BAD PATH +++"); \
        pltf_log(FILE_AND_LINE);      \
        assert(0);                    \
    } while (0);

typedef struct {
    i32 num;
    i32 den;
} ratio_i32;

typedef struct {
    i64 num;
    i64 den;
} ratio_i64;

typedef ratio_i32 ratio_s;

typedef struct {
    i32 x, y, w, h;
} rec_i32;

typedef struct {
    ALIGNAS(2)
    i8 x;
    i8 y;
} v2_i8;

typedef struct {
    ALIGNAS(4)
    i16 x; // word alignment for ARM intrinsics
    i16 y;
} v2_i16;

typedef struct {
    i32 x;
    i32 y;
} v2_i32;

typedef struct {
    f32 x;
    f32 y;
} v2_f32;

typedef struct {
    v2_i32 p[3];
} tri_i32;

typedef struct {
    v2_i16 p[3];
} tri_i16;

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

typedef struct { // A-----B    segment
    v2_i16 a;
    v2_i16 b;
} lineseg_i16;

typedef struct { // A-----B--- ray
    v2_i16 a;    //
    v2_i16 b;    // inf
} lineray_i16;

typedef struct { // ---A-----B--- line
    v2_i16 a;    // inf
    v2_i16 b;    // inf
} line_i16;

typedef struct {
    f32 m[9];
} m33_f32;

typedef struct {
    i32 n;
    i32 c;
    u8 *s;
} str_s;

static inline v2_i32 v2_i32_from_v2_i8(v2_i8 v)
{
    v2_i32 r = {v.x, v.y};
    return r;
}

static inline v2_i32 v2_i32_from_i16(v2_i16 a)
{
    v2_i32 r = {a.x, a.y};
    return r;
}

static inline v2_i16 v2_i16_from_i32(v2_i32 a)
{
    v2_i16 r = {(i16)a.x, (i16)a.y};
    return r;
}

static inline v2_f32 v2_f32_from_i32(v2_i32 a)
{
    v2_f32 r = {(f32)a.x, (f32)a.y};
    return r;
}

static inline v2_i32 v2_i32_from_f32(v2_f32 a)
{
    v2_i32 r = {(i32)(a.x + .5f), (i32)(a.y + .5f)};
    return r;
}

// =============================================================================
// B8(00001111) -> 0x0F
// B16(10000000, 00001111) -> 0x800F
#define BX(A)           GLUE(BIN_, A)
#define B2(A)           GLUE(0x, BX(GLUE(000000, A)))
#define B4(A)           GLUE(0x, BX(GLUE(0000, A)))
#define B8(A)           GLUE(0x, BX(A))
#define B16(A, B)       GLUE(0x, GLUE(BX(A), BX(B)))
#define B24(A, B, C)    GLUE(0x, GLUE(GLUE(BX(A), BX(B)), BX(C)))
#define B32(A, B, C, D) GLUE(GLUE(0x, GLUE(GLUE(BX(A), BX(B)), GLUE(BX(C), BX(D)))), U)

#define BIN_00000000 00
#define BIN_00000001 01
#define BIN_00000010 02
#define BIN_00000011 03
#define BIN_00000100 04
#define BIN_00000101 05
#define BIN_00000110 06
#define BIN_00000111 07
#define BIN_00001000 08
#define BIN_00001001 09
#define BIN_00001010 0A
#define BIN_00001011 0B
#define BIN_00001100 0C
#define BIN_00001101 0D
#define BIN_00001110 0E
#define BIN_00001111 0F
#define BIN_00010000 10
#define BIN_00010001 11
#define BIN_00010010 12
#define BIN_00010011 13
#define BIN_00010100 14
#define BIN_00010101 15
#define BIN_00010110 16
#define BIN_00010111 17
#define BIN_00011000 18
#define BIN_00011001 19
#define BIN_00011010 1A
#define BIN_00011011 1B
#define BIN_00011100 1C
#define BIN_00011101 1D
#define BIN_00011110 1E
#define BIN_00011111 1F
#define BIN_00100000 20
#define BIN_00100001 21
#define BIN_00100010 22
#define BIN_00100011 23
#define BIN_00100100 24
#define BIN_00100101 25
#define BIN_00100110 26
#define BIN_00100111 27
#define BIN_00101000 28
#define BIN_00101001 29
#define BIN_00101010 2A
#define BIN_00101011 2B
#define BIN_00101100 2C
#define BIN_00101101 2D
#define BIN_00101110 2E
#define BIN_00101111 2F
#define BIN_00110000 30
#define BIN_00110001 31
#define BIN_00110010 32
#define BIN_00110011 33
#define BIN_00110100 34
#define BIN_00110101 35
#define BIN_00110110 36
#define BIN_00110111 37
#define BIN_00111000 38
#define BIN_00111001 39
#define BIN_00111010 3A
#define BIN_00111011 3B
#define BIN_00111100 3C
#define BIN_00111101 3D
#define BIN_00111110 3E
#define BIN_00111111 3F
#define BIN_01000000 40
#define BIN_01000001 41
#define BIN_01000010 42
#define BIN_01000011 43
#define BIN_01000100 44
#define BIN_01000101 45
#define BIN_01000110 46
#define BIN_01000111 47
#define BIN_01001000 48
#define BIN_01001001 49
#define BIN_01001010 4A
#define BIN_01001011 4B
#define BIN_01001100 4C
#define BIN_01001101 4D
#define BIN_01001110 4E
#define BIN_01001111 4F
#define BIN_01010000 50
#define BIN_01010001 51
#define BIN_01010010 52
#define BIN_01010011 53
#define BIN_01010100 54
#define BIN_01010101 55
#define BIN_01010110 56
#define BIN_01010111 57
#define BIN_01011000 58
#define BIN_01011001 59
#define BIN_01011010 5A
#define BIN_01011011 5B
#define BIN_01011100 5C
#define BIN_01011101 5D
#define BIN_01011110 5E
#define BIN_01011111 5F
#define BIN_01100000 60
#define BIN_01100001 61
#define BIN_01100010 62
#define BIN_01100011 63
#define BIN_01100100 64
#define BIN_01100101 65
#define BIN_01100110 66
#define BIN_01100111 67
#define BIN_01101000 68
#define BIN_01101001 69
#define BIN_01101010 6A
#define BIN_01101011 6B
#define BIN_01101100 6C
#define BIN_01101101 6D
#define BIN_01101110 6E
#define BIN_01101111 6F
#define BIN_01110000 70
#define BIN_01110001 71
#define BIN_01110010 72
#define BIN_01110011 73
#define BIN_01110100 74
#define BIN_01110101 75
#define BIN_01110110 76
#define BIN_01110111 77
#define BIN_01111000 78
#define BIN_01111001 79
#define BIN_01111010 7A
#define BIN_01111011 7B
#define BIN_01111100 7C
#define BIN_01111101 7D
#define BIN_01111110 7E
#define BIN_01111111 7F
#define BIN_10000000 80
#define BIN_10000001 81
#define BIN_10000010 82
#define BIN_10000011 83
#define BIN_10000100 84
#define BIN_10000101 85
#define BIN_10000110 86
#define BIN_10000111 87
#define BIN_10001000 88
#define BIN_10001001 89
#define BIN_10001010 8A
#define BIN_10001011 8B
#define BIN_10001100 8C
#define BIN_10001101 8D
#define BIN_10001110 8E
#define BIN_10001111 8F
#define BIN_10010000 90
#define BIN_10010001 91
#define BIN_10010010 92
#define BIN_10010011 93
#define BIN_10010100 94
#define BIN_10010101 95
#define BIN_10010110 96
#define BIN_10010111 97
#define BIN_10011000 98
#define BIN_10011001 99
#define BIN_10011010 9A
#define BIN_10011011 9B
#define BIN_10011100 9C
#define BIN_10011101 9D
#define BIN_10011110 9E
#define BIN_10011111 9F
#define BIN_10100000 A0
#define BIN_10100001 A1
#define BIN_10100010 A2
#define BIN_10100011 A3
#define BIN_10100100 A4
#define BIN_10100101 A5
#define BIN_10100110 A6
#define BIN_10100111 A7
#define BIN_10101000 A8
#define BIN_10101001 A9
#define BIN_10101010 AA
#define BIN_10101011 AB
#define BIN_10101100 AC
#define BIN_10101101 AD
#define BIN_10101110 AE
#define BIN_10101111 AF
#define BIN_10110000 B0
#define BIN_10110001 B1
#define BIN_10110010 B2
#define BIN_10110011 B3
#define BIN_10110100 B4
#define BIN_10110101 B5
#define BIN_10110110 B6
#define BIN_10110111 B7
#define BIN_10111000 B8
#define BIN_10111001 B9
#define BIN_10111010 BA
#define BIN_10111011 BB
#define BIN_10111100 BC
#define BIN_10111101 BD
#define BIN_10111110 BE
#define BIN_10111111 BF
#define BIN_11000000 C0
#define BIN_11000001 C1
#define BIN_11000010 C2
#define BIN_11000011 C3
#define BIN_11000100 C4
#define BIN_11000101 C5
#define BIN_11000110 C6
#define BIN_11000111 C7
#define BIN_11001000 C8
#define BIN_11001001 C9
#define BIN_11001010 CA
#define BIN_11001011 CB
#define BIN_11001100 CC
#define BIN_11001101 CD
#define BIN_11001110 CE
#define BIN_11001111 CF
#define BIN_11010000 D0
#define BIN_11010001 D1
#define BIN_11010010 D2
#define BIN_11010011 D3
#define BIN_11010100 D4
#define BIN_11010101 D5
#define BIN_11010110 D6
#define BIN_11010111 D7
#define BIN_11011000 D8
#define BIN_11011001 D9
#define BIN_11011010 DA
#define BIN_11011011 DB
#define BIN_11011100 DC
#define BIN_11011101 DD
#define BIN_11011110 DE
#define BIN_11011111 DF
#define BIN_11100000 E0
#define BIN_11100001 E1
#define BIN_11100010 E2
#define BIN_11100011 E3
#define BIN_11100100 E4
#define BIN_11100101 E5
#define BIN_11100110 E6
#define BIN_11100111 E7
#define BIN_11101000 E8
#define BIN_11101001 E9
#define BIN_11101010 EA
#define BIN_11101011 EB
#define BIN_11101100 EC
#define BIN_11101101 ED
#define BIN_11101110 EE
#define BIN_11101111 EF
#define BIN_11110000 F0
#define BIN_11110001 F1
#define BIN_11110010 F2
#define BIN_11110011 F3
#define BIN_11110100 F4
#define BIN_11110101 F5
#define BIN_11110110 F6
#define BIN_11110111 F7
#define BIN_11111000 F8
#define BIN_11111001 F9
#define BIN_11111010 FA
#define BIN_11111011 FB
#define BIN_11111100 FC
#define BIN_11111101 FD
#define BIN_11111110 FE
#define BIN_11111111 FF

#endif