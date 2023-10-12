// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef OS_TYPES_H
#define OS_TYPES_H

#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* When compiled one of the following symbols is defined:
 * OS_DESKTOP    - Desktop build raylib
 * OS_SIMULATOR        - Playdate simulator
 * OS_HARDWARE_DEBUG   - Playdate hardware debug
 * OS_HARDWARE_RELEASE - Playdate hardware release
 * OS_HARDWARE         - Playdate hardware (defined by one of the above)
 * OS_PLAYDATE   - defined by either sim or hw
 */

// this is to enable editing Playdate specific stuff
// and disable Visual Studio's definitions
#if !defined(OS_PLAYDATE) && defined(OS_DESKTOP) && 0
#undef OS_DESKTOP
#define OS_PLAYDATE
#define TARGET_PD
#define TARGET_EXTENSION 1
#endif

#if defined(OS_HARDWARE_DEBUG) || defined(OS_HARDWARE_RELEASE)
#define OS_HARDWARE
#endif

#define STATIC_ASSERT static_assert
#ifndef OS_HARDWARE
#define ASSERT assert
#else
#define ASSERT(X) // disable assert on hardware bc unimplemented
#endif

#ifdef OS_DESKTOP
// RAYLIB ======================================================================
#include "include/raylib.h"

#define os_time (float)GetTime
#define PRINTF  printf
#endif

#ifdef OS_PLAYDATE
// PLAYDATE ====================================================================
#include "pd_api.h"
#define os_time PD_elapsedtime

extern PlaydateAPI *PD;
extern void (*PD_log)(const char *fmt, ...);
extern float (*PD_elapsedtime)();
extern int (*PD_ftell)(SDFile *file);
extern int (*PD_fread)(SDFile *file, void *buf, unsigned int len);
extern int (*PD_fseek)(SDFile *file, int pos, int whence);
extern int (*PD_fwrite)(SDFile *file, const void *buf, unsigned int len);
extern SDFile *(*PD_fopen)(const char *path, FileOptions mode);
extern int (*PD_fclose)(SDFile *file);

#ifdef OS_HARDWARE
#define PRINTF(C, ...) // disable printf calls
#else
#define PRINTF PD_log
#endif
//
#endif

static void print_bin_u32(char *buf, unsigned int v)
{
        for (int i = 0; i < 32; i++) {
                if ((v & (1U << (31 - i)))) {
                        buf[i] = '1';
                } else {
                        buf[i] = '0';
                }
        }
}
// =============================================================================

typedef unsigned char  uchar;
typedef unsigned short ushort;
typedef unsigned int   uint;
typedef unsigned long  ulong;
typedef int8_t         i8;
typedef int16_t        i16;
typedef int32_t        i32;
typedef int64_t        i64;
typedef uint8_t        u8;
typedef uint16_t       u16;
typedef uint32_t       u32;
typedef uint64_t       u64;
typedef int8_t         bool8;
typedef int16_t        bool16;
typedef int32_t        bool32;
typedef float          f32;
typedef u8             flags8;
typedef u16            flags16;
typedef u32            flags32;
typedef u64            flags64;

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

#define NOT_IMPLEMENTED  ASSERT(0);
#define BAD_PATH         ASSERT(0);
#define GLUE2(A, B)      A##B
#define GLUE(A, B)       GLUE2(A, B)
#define ARRLEN(A)        (sizeof(A) / sizeof(A[0]))
#define MAX(A, B)        ((A) >= (B) ? (A) : (B))
#define MIN(A, B)        ((A) <= (B) ? (A) : (B))
#define ABS(A)           ((A) >= 0 ? (A) : -(A))
#define SGN(A)           ((0 < (A)) - (0 > (A)))
#define CLAMP(X, LO, HI) ((X) > (HI) ? (HI) : ((X) < (LO) ? (LO) : (X)))
#define SWAP(T, a, b)          \
        do {                   \
                T tmp_ = a;    \
                a      = b;    \
                b      = tmp_; \
        } while (0)
#define FILE_AND_LINE__(A, B) A "|" #B
#define FILE_AND_LINE_(A, B)  FILE_AND_LINE__(A, B)
#define FILE_AND_LINE         FILE_AND_LINE_(__FILE__, __LINE__)
#define ALIGNAS               _Alignas

#ifdef __GNUC__
#define FORCE_INLINE __attribute__((always_inline)) inline
#else
#define FORCE_INLINE inline
#endif

#ifdef OS_HARDWARE

static FORCE_INLINE u32 bswap32(u32 v)
{
        u32 r;
        __asm("rev %0, %1"
              : "=r"(r)
              : "r"(v));
        return r;
}

static FORCE_INLINE u32 log2_32(u32 v)
{
        i32 r;
        __asm("clz %0, %1"
              : "=r"(r)
              : "r"(v));
        return (31 - r);
}

static FORCE_INLINE u32 brev32(u32 v)
{
        u32 r;
        __asm("rbit %0, %1"
              : "=r"(r)
              : "r"(v));
        return r;
}

// adds two signed 16 bit integers
static FORCE_INLINE u32 sadd16(u32 a, u32 b)
{
        u32 r;
        __asm("sadd16 %0, %1, %2"
              : "=r"(r)
              : "r"(a), "r"(b));
        return r;
}

static FORCE_INLINE f32 sqrt_f32(f32 v)
{
        f32 r;
        __asm("vsqrt.f32 %0, %1"
              : "=t"(r)
              : "t"(v));
        return r;
}

#else
static inline u32 sadd16(u32 a, u32 b)
{
        u32  r  = 0;
        i16 *aa = (i16 *)&a;
        i16 *bb = (i16 *)&b;
        i16 *rr = (i16 *)&r;
        rr[0]   = aa[0] + bb[0];
        rr[1]   = aa[1] + bb[1];
        return r;
}

#define sqrt_f32 sqrtf

static inline u32 bswap32(u32 i)
{
        return (i >> 24) | ((i << 8) & 0xFF0000U) |
               (i << 24) | ((i >> 8) & 0x00FF00U);
}

static inline int log2_32(u32 x)
{
        static const u32 logtab[32] = {
            0, 9, 1, 10, 13, 21, 2, 29, 11, 14, 16, 18, 22, 25, 3, 30,
            8, 12, 20, 28, 15, 17, 24, 7, 19, 27, 23, 6, 26, 5, 4, 31};
        u32 v = x;
        v |= v >> 1, v |= v >> 2, v |= v >> 4, v |= v >> 8, v |= v >> 16;
        return logtab[(v * 0x7C4ACDDU) >> 27];
}

static inline u32 brev32(u32 x)
{
        u32 r = x;
        int s = 31;
        for (u32 v = x >> 1; v; v >>= 1) {
                r <<= 1;
                r |= v & 1;
                s--;
        }
        r <<= s;
        return r;
}
#endif

static inline f32 sqrt_f32_math(f32 v)
{
        return sqrtf(v);
}

// =============================================================================
#define char_isdigit(C)   ('0' <= (C) && (C) <= '9')
#define char_digit        char_isdigit
#define char_digit_1_9(C) ('1' <= (C) && (C) <= '9')

static int char_hex_to_int(char c)
{
        switch (c) {
        case '0': return 0;
        case '1': return 1;
        case '2': return 2;
        case '3': return 3;
        case '4': return 4;
        case '5': return 5;
        case '6': return 6;
        case '7': return 7;
        case '8': return 8;
        case '9': return 9;
        case 'A':
        case 'a': return 10;
        case 'B':
        case 'b': return 11;
        case 'C':
        case 'c': return 12;
        case 'D':
        case 'd': return 13;
        case 'E':
        case 'e': return 14;
        case 'F':
        case 'f': return 15;
        }
        return 0;
}

static bool32 char_matches_any(const char c, const char *chars)
{
        for (const char *cc = chars; *cc != '\0'; cc++) {
                if (*cc == c) return 1;
        }
        return 0;
}

#define streq(A, B) (strcmp(A, B) == 0)
#define os_strlen   strlen
#define os_strcpy   strcpy // copies src to dst INCLUDING 0 character
#define os_strcat   strcat

// copies src to dst EXCLUDING 0 character
static char *os_strncpy(char *dst, const char *src)
{
        ASSERT(dst && src);
        for (int i = 0;; i++) {
                if (src[i] == '\0') break;
                dst[i] = src[i];
        }
        return dst;
}

static char *os_strcat_i32(char *s1, i32 value)
{
        if (!s1) return s1;

        if (value == 0) {
                *s1 = '0';
                return s1;
        }

        char *s = s1;
        while (*s != '\0') {
                s++;
        }

        i32 num = value;
        if (num < 0) {
                num  = -num;
                *s++ = '-';
        }

        for (i32 temp = num; temp > 0; temp /= 10) {
                s++;
        }

        *s = '\0';
        for (i32 temp = num; temp > 0; temp /= 10) {
                *--s = temp % 10 + '0';
        }
        return s1;
}

static i32 os_i32_from_str(const char *s)
{
        int i = 0;
        while (1) {
                char c = s[i];
                if (c == '\0') return 0;
                if (char_matches_any(c, " \n\t"))
                        i++;
                else
                        break;
        }

        i32 res = 0;
        i32 sig = 1;
        switch (s[i]) {
        case '-': sig = -1;
        case '+': i++; break;
        }

        while (1) {
                char c = s[i];
                if (!char_digit(c)) break;
                res *= 10;
                res += (int)(c - '0');
                i++;
        }
        return (res * sig);
}

static inline int ptr_find(const void *ptr, const void **parr, int n)
{
        for (int i = 0; i < n; i++) {
                if (ptr == parr[i])
                        return i;
        }
        return -1;
}

#define _BG2(A, B) A##B
#define _BG(A, B)  _BG2(A, B)

// B8(00001111) -> 0x0F
// B16(10000000, 00001111) -> 0x800F
#define _B_(A)       _BG(_B, A)
#define B8(A)        _BG(0x, _B_(A))
#define B16(A, B)    _BG(0x, _BG(_B_(A), _B_(B)))
#define B24(A, B, C) _BG(0x, _BG(_BG(_B_(A), _B_(B)), _B_(C)))
#define B32(A, B, C, D) \
        _BG(_BG(0x, _BG(_BG(_B_(A), _B_(B)), _BG(_B_(C), _B_(D)))), U)

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