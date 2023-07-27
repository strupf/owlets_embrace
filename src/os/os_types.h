#ifndef OS_TYPES_H
#define OS_TYPES_H

// this is to enable editing Playdate specific stuff
// and disable Visual Studio's definitions

#if 0
#undef TARGET_DESKTOP
#define TARGET_PD
#define TARGET_EXTENSION 1
#endif

#if defined(TARGET_DESKTOP)
// RAYLIB ======================================================================
#include "include/raylib.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define os_time       GetTime
#define ASSERT        assert
#define STATIC_ASSERT static_assert
#define PRINTF        printf
#elif defined(TARGET_PD)
// PLAYDATE ====================================================================
#include "pd_api.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#define os_time PD->system->getElapsedTime

extern PlaydateAPI *PD;
extern float (*PD_crank)(void);
extern int (*PD_crankdocked)(void);
extern void (*PD_buttonstate)(PDButtons *, PDButtons *, PDButtons *);
extern void (*PD_log)(const char *fmt, ...);
#define ASSERT(E)
#define STATIC_ASSERT(E, M)
#define PRINTF PD_log
//
#endif
// =============================================================================

typedef unsigned short ushort;
typedef unsigned char  uchar;
typedef unsigned int   uint;
typedef int8_t         i8;
typedef int16_t        i16;
typedef int32_t        i32;
typedef int64_t        i64;
typedef uint8_t        u8;
typedef uint16_t       u16;
typedef uint32_t       u32;
typedef uint64_t       u64;
typedef int32_t        bool32;

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

#endif