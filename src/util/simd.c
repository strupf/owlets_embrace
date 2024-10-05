// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "simd.h"

static inline vec32 vec32_ldu(const void *x)
{
    assert(x);
#if SIMD_USE_TYPE_PUNNING
    vec32 v = *(vec32 *)&x;
#else
    vec32 v = {0};
    mcpy(&v.u_32, x, 4); // this will generate a simple LDR instruction on -O2
#endif
    return v;
}

static inline void vec32_stu(vec32 v, void *x)
{
    assert(x);
#if SIMD_USE_TYPE_PUNNING
    *(vec32 *)x = v;
#else
    mcpy(x, &v.u_32, 4); // this will generate a simple STR instruction on -O2
#endif
}

static inline void vec32_unpack(vec32 v, void *r)
{
    assert(r);
#if SIMD_USE_TYPE_PUNNING
    *(vec32 *)r = v;
#else
    mcpy(r, &v.u_32, 4); // this will generate a simple STR instruction on -O2
#endif
}

static inline vec32 u16x2_pack(u16 x, u16 y)
{
    vec32 r   = {0};
    r.u_16[0] = x;
    r.u_16[1] = y;
    SIMD_END_U16(r);
    return r;
}

static inline vec32 i16x2_pack(i16 x, i16 y)
{
    vec32 r   = {0};
    r.i_16[0] = x;
    r.i_16[1] = y;
    SIMD_END_I16(r);
    return r;
}

static inline vec32 u8x4_pack(u8 a, u8 b, u8 c, u8 d)
{
    vec32 r  = {0};
    r.u_8[0] = a;
    r.u_8[1] = b;
    r.u_8[2] = c;
    r.u_8[3] = d;
    SIMD_END_U8(r);
    return r;
}

static inline vec32 i8x4_pack(i8 a, i8 b, i8 c, i8 d)
{
    vec32 r  = {0};
    r.i_8[0] = a;
    r.i_8[1] = b;
    r.i_8[2] = c;
    r.i_8[3] = d;
    SIMD_END_I8(r);
    return r;
}

static inline vec32 vec32_from_v2_i16(v2_i16 x)
{
#if SIMD_USE_TYPE_PUNNING
    vec32 v = *(vec32 *)&x;
#else
    vec32 v   = {0};
    v.i_16[0] = x.x;
    v.i_16[1] = x.y;
    SIMD_END_I16(v);
#endif
    return v;
}

static inline v2_i16 v2_i16_from_vec32(vec32 v)
{
#if SIMD_USE_TYPE_PUNNING
    v2_i16 x = *(v2_i16 *)&v;
#else
    SIMD_BEG_I16(v);
    v2_i16 x = {v.i_16[0], v.i_16[1]};
#endif
    return x;
}