// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef SIMD_H
#define SIMD_H

#include "pltf/pltf_types.h"

typedef union vec32 {
    u32 u_32;
    u16 u_16[2];
    i16 i_16[2];
    u8  u_8[4];
    i8  i_8[4];
} vec32;

typedef vec32 u8x4;
typedef vec32 i8x4;
typedef vec32 u16x2;
typedef vec32 i16x2;

#define SIMD_USE_TYPE_PUNNING 0

#if SIMD_USE_TYPE_PUNNING // use type punning through the union
#define simd_prep(D, S)
#else
// explicitly initialize union fields for type punning
// -O2 removes it entirely
static inline void simd_prep(void *d, void *s)
{
    ((byte *)d)[0] = ((byte *)s)[0];
    ((byte *)d)[1] = ((byte *)s)[1];
    ((byte *)d)[2] = ((byte *)s)[2];
    ((byte *)d)[3] = ((byte *)s)[3];
}
#endif

#define SIMD_BEG_I16(V) simd_prep(&V.i_16[0], &V.u_32)
#define SIMD_BEG_U16(V) simd_prep(&V.u_16[0], &V.u_32)
#define SIMD_BEG_I8(V)  simd_prep(&V.i_8[0], &V.u_32)
#define SIMD_BEG_U8(V)  simd_prep(&V.u_8[0], &V.u_32)
#define SIMD_END_I16(V) simd_prep(&V.u_32, &V.i_16[0])
#define SIMD_END_U16(V) simd_prep(&V.u_32, &V.u_16[0])
#define SIMD_END_I8(V)  simd_prep(&V.u_32, &V.i_8[0])
#define SIMD_END_U8(V)  simd_prep(&V.u_32, &V.u_8[0])

#define vec32_ld             vec32_ldu
#define vec32_st             vec32_stu
#define u16x2_unpack(V, ARR) vec32_unpack
#define i16x2_unpack(V, ARR) vec32_unpack
#define u8x4_unpack(V, ARR)  vec32_unpack
#define i8x4_unpack(V, ARR)  vec32_unpack

static inline vec32  vec32_ldu(const void *x);
static inline void   vec32_stu(vec32 v, void *x);
//
static inline i32    i16x2_muad(vec32 a, vec32 b);  // x1 * x2 + y1 * y2
static inline i32    i16x2_musd(vec32 a, vec32 b);  // x1 * x2 - y1 * y2
static inline i32    i16x2_muadx(vec32 a, vec32 b); // x1 * y2 + y1 * x2
static inline i32    i16x2_musdx(vec32 a, vec32 b); // x1 * y2 - y1 * x2
static inline vec32  i16x2_add(vec32 a, vec32 b);
static inline vec32  i16x2_sub(vec32 a, vec32 b);
static inline vec32  i16x2_adds(vec32 a, vec32 b);
static inline vec32  i16x2_subs(vec32 a, vec32 b);
//
static inline vec32  u16x2_add(vec32 a, vec32 b);
static inline vec32  u16x2_sub(vec32 a, vec32 b);
static inline vec32  u16x2_adds(vec32 a, vec32 b);
static inline vec32  u16x2_subs(vec32 a, vec32 b);
//
static inline void   vec32_unpack(vec32 v, void *r);
static inline vec32  u16x2_pack(u16 x, u16 y);
static inline vec32  i16x2_pack(i16 x, i16 y);
static inline vec32  u8x4_pack(u8 a, u8 b, u8 c, u8 d);
static inline vec32  i8x4_pack(i8 a, i8 b, i8 c, i8 d);
//
static inline vec32  vec32_from_v2_i16(v2_i16 x);
static inline v2_i16 v2_i16_from_vec32(vec32 v);
#endif