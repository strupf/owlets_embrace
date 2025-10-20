// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef PLTF_TYPES_H
#define PLTF_TYPES_H

#include "pltf/pltf_config.h"

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
void (*PD_system_error)(const char *format, ...);
#if 1
#undef assert
#define assert(X)
#else
#undef assert
#define assert(X)                                \
    do {                                         \
        if (!(X)) {                              \
            pltf_log("assertion hit: " #X "\n"); \
            PD_system_error("ASSERT: " #X "\n"); \
        }                                        \
    } while (0)
#endif
#elif PLTF_SDL && 1 // use SDL assert instead of std?
#include "pltf/SDL2/SDL_assert.h"
#undef assert
#define assert SDL_assert
#endif

#if !PLTF_DEBUG
#undef assert
#define assert(X)
#endif

typedef unsigned char  byte;
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
#define Q_VOBJ(X)   Q_X(X, 4096.f)
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

#if (201112L <= __STDC_VERSION__)
#if (__STDC_VERSION__ < 202311L) // earler than C23
#include <stdalign.h>
#endif
#else
#error C VERSION NOT SUPPORTED
#endif
#endif

#define OFFSETOF      offsetof
#define ALIGNAS       alignas
#define ALIGNOF       alignof
#define ALIGNAST(T)   ALIGNAS(ALIGNOF(T))
#define ALIGNTO(X, A) (((X) + (A) - 1) & ~((A) - 1))

// used for user defined allocations
// alloc(ctx, size) -> ctx: pointer to some memory manager
typedef struct {
    ALIGNAS(8)
    void *(*allocfunc)(void *ctx, usize s, usize alignment);
    void *ctx;
} allocator_s;

static inline void *allocator_alloc(allocator_s a, usize s, usize alignment)
{
    if (!a.allocfunc) return 0;
    return (a.allocfunc(a.ctx, s, alignment));
}

// clang-format off
#define typedef_struct(NAME)  typedef struct NAME NAME; struct NAME
#define typedef_union(NAME)   typedef union NAME NAME; union NAME
#define mset                  memset
#define mcpy                  memcpy
#define mmov                  memmove
#define mclr(DST, SIZE)       mset(DST, 0, SIZE)
#define mclr_static_arr(DST)  mclr(DST, sizeof(DST))
#define mclr_field(DST)       mclr(&(DST), sizeof(DST))
#define mclr_ptr(DST)         mclr(DST, sizeof(*(DST)))
#define POW2(X)               ((X) * (X))
#define GLUE2(A, B)           A##B
#define GLUE(A, B)            GLUE2(A, B)
#define ARRLEN(A)             (i32)(sizeof(A) / sizeof(A[0]))
#define SWAP(T, A, B)         do { T tmp_var = A; A = B; B = tmp_var; } while(0)
#define MKILOBYTE(X)          ((X) * 1024)
#define MMEGABYTE(X)          ((X) * 1024 * 1024)
#define FILE_AND_LINE__(A, B) A "|" #B
#define FILE_AND_LINE_(A, B)  FILE_AND_LINE__(A, B)
#define FILE_AND_LINE         FILE_AND_LINE_(__FILE__, __LINE__)
#define IS_POW2(X)            (((X) & ((X) - 1)) == 0)
#define EMPTY_LOOP            do {} while (0)

#if PLTF_DEBUG
#define DEBUG_CODE(X) X
#define DEBUG_ASSERT(X) assert(X)
#else
#define DEBUG_CODE(X)
#define DEBUG_ASSERT(X) 
#endif


#ifdef __GNUC__
#define ATTRIBUTE_SECTION(X)      __attribute__((section(X)))
#define PREFETCH(ADDR)            __builtin_prefetch(ADDR)
#define PTR_ALIGNED(P, ALIGNMENT) __builtin_assume_aligned(P, ALIGNMENT)
#define LIKELY(X)                 __builtin_expect(!!(X), 1)
#define UNLIKELY(X)               __builtin_expect(!!(X), 0)
#else
#define ATTRIBUTE_SECTION(X)
#define PREFETCH(ADDR)            EMPTY_LOOP
#define PTR_ALIGNED(P, ALIGNMENT) (P)
#define LIKELY(X)                 (X)
#define UNLIKELY(X)               (X)
#endif
// clang-format on

#define NOT_IMPLEMENTED(X)                        \
    do {                                          \
        pltf_log("+++ NOT IMPLEMENTED: " X "\n"); \
        pltf_log(FILE_AND_LINE);                  \
        assert(0);                                \
    } while (0)
#define BAD_PATH()                \
    do {                          \
        pltf_log("+++ BAD PATH"); \
        pltf_log(FILE_AND_LINE);  \
        assert(0);                \
    } while (0)

static inline void *align_ptr(void *p, usize alignment)
{
    assert(IS_POW2(alignment));
    return (void *)ALIGNTO((uptr)p, alignment);
}

static inline usize align_usize(usize s, usize alignment)
{
    assert(IS_POW2(alignment));
    return ALIGNTO(s, alignment);
}

// BSWAP
#if defined(__GNUC__)
#define bswap16 __builtin_bswap16
#define bswap32 __builtin_bswap32
#define bswap64 __builtin_bswap64
#elif defined(_MSC_VER)
#define bswap16 _byteswap_ushort
#define bswap32 _byteswap_ulong
#define bswap64 _byteswap_uint64
#else
static u32 bswap16(u16 i)
{
    return (i >> 24) | ((i << 8) & 0xFF0000U) |
           (i << 24) | ((i >> 8) & 0x00FF00U);
}

static u32 bswap32(u32 i)
{
    return (i >> 24) | ((i << 8) & 0xFF0000U) |
           (i << 24) | ((i >> 8) & 0x00FF00U);
}

static u64 bswap64(u64 i)
{
    return ((i >> 56) & 0x00000000000000FFULL) | ((i >> 8) & 0x00000000FF000000ULL) |
           ((i << 56) & 0xFF00000000000000ULL) | ((i << 8) & 0x000000FF00000000ULL) |
           ((i >> 40) & 0x000000000000FF00ULL) | ((i >> 24) & 0x0000000000FF0000ULL) |
           ((i << 40) & 0x00FF000000000000ULL) | ((i << 24) & 0x0000FF0000000000ULL);
}
#endif

#define PLTF_ENDIAN_LE 1
#define PLTF_ENDIAN    PLTF_ENDIAN_LE

#ifdef PLTF_ENDIAN_LE
#define PLTF_ENDIAN_BE (1 - PLTF_ENDIAN_LE)
#else
#define PLTF_ENDIAN_LE (1 - PLTF_ENDIAN_BE)
#endif

#if PLTF_ENDIAN == PLTF_ENDIAN_LE
#define BSWAP16_IF_NEEDED(X) (X)
#define BSWAP32_IF_NEEDED(X) (X)
#define BSWAP64_IF_NEEDED(X) (X)
#else
#define BSWAP16_IF_NEEDED(X) bswap16(X)
#define BSWAP32_IF_NEEDED(X) bswap32(X)
#define BSWAP64_IF_NEEDED(X) bswap64(X)
#endif

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