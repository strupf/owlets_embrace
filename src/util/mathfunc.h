// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef MATHFUNC_H
#define MATHFUNC_H

#include "pltf/pltf.h"

#define PI_FLOAT  3.1415927f
#define PI2_FLOAT 6.2831853f

#define QX_FRAC(Q, D, N) (((D) << (Q)) / (N))
#define Q8_FRAC(D, N)    QX_FRAC(8, D, N)
#define Q12_FRAC(D, N)   QX_FRAC(12, D, N)
#define Q16_FRAC(D, N)   QX_FRAC(16, D, N)

static const i32 cos_table[256];

#define clamp_i clamp_i32
#define clamp_f clamp_f32
#define sgn_i   sgn_i32
#define sgn_f   sgn_f32
#define max_i   max_i32
#define max_f   max_f32
#define min_i   min_i32
#define min_f   min_f32
#define abs_i   abs_i32
#define abs_f   abs_f32

static inline i32 clamp_i32(i32 x, i32 lo, i32 hi)
{
    Q8_FRAC(1, 256);
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

static inline f32 clamp_f32(f32 x, f32 lo, f32 hi)
{
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

static inline i32 min_i32(i32 a, i32 b)
{
    return (a < b ? a : b);
}

static inline u32 min_u32(u32 a, u32 b)
{
    return (a < b ? a : b);
}

static inline i32 min3_i32(i32 a, i32 b, i32 c)
{
    return min_i32(a, min_i32(b, c));
}

static inline f32 min_f32(f32 a, f32 b)
{
    return (a < b ? a : b);
}

static inline i32 max_i32(i32 a, i32 b)
{
    return (a > b ? a : b);
}

static inline i32 max3_i32(i32 a, i32 b, i32 c)
{
    return max_i32(a, max_i32(b, c));
}

static inline f32 max_f32(f32 a, f32 b)
{
    return (a > b ? a : b);
}

static inline i32 abs_i32(i32 a)
{
    return (a < 0 ? -a : a);
}

static inline f32 abs_f32(f32 a)
{
    return (a < 0.f ? -a : a);
}

static inline i32 sgn_i32(i32 a)
{
    if (a < 0) return -1;
    if (a > 0) return +1;
    return 0;
}

static inline f32 sgn_f32(f32 a)
{
    if (a < 0.f) return -1.f;
    if (a > 0.f) return +1.f;
    return 0.f;
}

static inline i32 mul_ratio(i32 x, ratio_s r)
{
    return ((x * r.num) / r.den);
}

static inline u32 pow2_u32(u32 v)
{
    return (v * v);
}

static inline i32 pow2_i32(i32 v)
{
    return (v * v);
}

static inline f32 pow_f32(f32 v, i32 power)
{
    f32 r = v;
    for (i32 n = 1; n < power; n++) {
        r *= v;
    }
    return r;
}

static inline u32 pow_u32(u32 v, i32 power)
{
    u32 r = v;
    for (i32 n = 1; n < power; n++) {
        r *= v;
    }
    return r;
}

static inline i32 pow_i32(i32 v, i32 power)
{
    i32 r = v;
    for (i32 n = 1; n < power; n++) {
        r *= v;
    }
    return r;
}

static inline i32 log2_u32(u32 v)
{
    return (31 - clz32(v));
}

static inline bool32 is_pow2_u32(u32 v)
{
    return ((v & (v - 1)) == 0);
}

// convert fixed point numbers without rounding
static inline i32 q_convert_i32(i32 v, i32 qfrom, i32 qto)
{
    if (qfrom < qto) return (v >> (qto - qfrom));
    if (qfrom > qto) return (v << (qfrom - qto));
    return v;
}

// convert fixed point numbers without rounding
static inline u32 q_convert_u32(u32 v, i32 qfrom, i32 qto)
{
    if (qfrom < qto) return (v >> (qto - qfrom));
    if (qfrom > qto) return (v << (qfrom - qto));
    return v;
}

// rounded division - stackoverflow.com/a/18067292
static inline i32 divr_i32(i32 n, i32 d)
{
    i32 h = d / 2;
    return ((n ^ d) < 0 ? (n - h) / d : (n + h) / d);
}

static inline i32 lerp_i32(i32 a, i32 b, i32 num, i32 den)
{
    i32 i = (b - a) * num;
#ifdef PLTF_SDL
    i64 j = (i64)(b - a) * (i64)num;
    assert((i64)i == j);
#endif
    return (a + i / den);
}

// more expensive variant using an immediate u64 to prevent overflow
static inline i32 lerpl_i32(i32 a, i32 b, i32 num, i32 den)
{
    i32 r = a + (i32)(((i64)(b - a) * (i64)num) / (i64)den);
    return r;
}

static inline f32 lerp_f32(f32 a, f32 b, f32 r)
{
    return (a + (b - a) * r);
}

static inline bool32 between_excl_incl_i32(i32 x, i32 a, i32 b)
{
    return (a < x && x <= b) || (b <= x && x < a);
}

static inline bool32 between_incl_excl_i32(i32 x, i32 a, i32 b)
{
    return (a <= x && x < b) || (b < x && x <= a);
}

static inline bool32 between_excl_i32(i32 x, i32 a, i32 b)
{
    return (a < x && x < b) || (b < x && x < a);
}

static inline bool32 between_incl_i32(i32 x, i32 a, i32 b)
{
    return (a <= x && x <= b) || (b <= x && x <= a);
}

static inline f32 sqrt_f32(f32 x)
{
    return sqrtf(x);
}

static inline u32 sqrt_u64(u64 x)
{
    if (x >= U64_C(0xFFFFFFFE00000001)) return U32_C(0xFFFFFFFF);
    if (x == 0) return 0;
    u64 v = (u64)sqrt_f32((f32)x);
    v     = (v + x / v) >> 1;
    return (u32)((v * v > x) ? v - 1 : v);
}

static inline u32 sqrt_u32(u32 x)
{
#if 1
    if (x >= U32_C(0xFFFE0001)) return 0xFFFF;
    u32 v = (u32)sqrt_f32((f32)x);
    // v     = (v + x / v) >> 1;
    return ((v * v > x) ? v - 1 : v);
#else
    u32 y = 0;
    for (u32 z = x, m = 0x40000000 >> (clz32(x) << 1); m; m >>= 2) {
        u32 b = y | m;
        y >>= 1;
        if (b <= z) {
            z -= b;
            y |= m;
        }
    }
    return y;
#endif
}

static inline u32 sqrt_u32_exact(u32 x)
{
    u32 y = 0;
    for (u32 z = x, m = 0x40000000 >> (clz32(x) << 1); m; m >>= 2) {
        u32 b = y | m;
        y >>= 1;
        if (b <= z) {
            z -= b;
            y |= m;
        }
    }
    return y;
}

static inline i32 sqrt_i32(i32 x)
{
#ifdef PLTF_DEBUG
    if (x < 0) {
        pltf_log("sqrt_warn: negative number!\n");
    }
#endif
    return (x <= 0 ? 0 : (i32)sqrt_u32(x));
}

#define Q16_ANGLE_TURN 0x40000

static i32 turn_q18_calc(i32 num, i32 den)
{
    return (0x40000 * num) / den;
}

// p: angle/turn, where 2 PI or 1 turn = 262144 (0x40000)
// output: [-65536, 65536] = [-1; +1]
static i32 cos_q16(i32 turn_q18)
{
    i32 i   = (u32)(turn_q18 >> 8) & 0x3FF;
    i32 neg = 0;
    switch (i & 0x300) {                       // [0, 256)
    case 0x100: i = 0x200 - i, neg = 1; break; // [256, 512)
    case 0x200: i = i - 0x200, neg = 1; break; // [512, 768)
    case 0x300: i = 0x400 - i; break;          // [768, 1024)
    }
    if (i == 0x100) return 0;
    i32 r = cos_table[i];
    return neg ? -r : r;
}

// p: angle, where 262144 = 0x40000 = 2 PI
// output: [-65536, 65536] = [-1; +1]
static inline i32 sin_q16(i32 turn_q18)
{
    return cos_q16(turn_q18 - 0x10000);
}

// p: angle, where 1024 = 0x400 = 2 PI
// output: [-64, 64] = [-1; +1]
static inline i32 cos_q6(i32 turn_q10)
{
    return (cos_q16(turn_q10 << 8) >> 10);
}

// p: angle, where 1024 = 0x400 = 2 PI
// output: [-64, 64] = [-1; +1]
static inline i32 sin_q6(i32 turn_q10)
{
    return (cos_q16((turn_q10 - 0x100) << 8) >> 10);
}

// input: [0,65536] = [0,1]
// output: [0, 32768] = [0, PI/4], 65536 = PI/2
static i32 atan_q16(i32 x)
{
    if (x == 0) return 0;

    // taylor expansion works for x [0, 1]
    // for bigger x: atan(x) = PI/2 - atan(1/x)
    i32 add = 0, mul = +1;
    u32 i = (u32)abs_i32(x);
    if (i > 0x10000) {
        i   = 0xFFFFFFFFU / i;
        add = 0x10000;
        mul = -1;
    }

    if (i == 0x10000) return (x > 0 ? +0x8000 : -0x8000); // atan(1) = PI/4
    u32 i2 = (i * i + 0x8000) >> 16;
    u32 r;                          // magic constants roughly:
    r = 0x00A97;                    // 1/13 + E
    r = 0x017AA - ((r * i2) >> 16); // 1/11
    r = 0x01CE0 - ((r * i2) >> 16); // 1/9
    r = 0x024F6 - ((r * i2) >> 16); // 1/7
    r = 0x0336F - ((r * i2) >> 16); // 1/5
    r = 0x05587 - ((r * i2) >> 16); // 1/3
    r = 0x10050 - ((r * i2) >> 16); // 1
    r = (r * i) >> 16;
    r = (r * 0x0A2FB) >> 16; // divide by PI/2 - 0x0A2FB ~ 0xFFFFFFFFu / (PI/2 << 16)

    i32 res = add + mul * (i32)r;
    return (x > 0 ? +res : -res);
}

// INPUT:  [-0x10000,0x10000] = [-1;1]
// OUTPUT: [-0x10000, 0x10000] = [-PI/2;PI/2]
static i32 asin_q16(i32 x)
{
    // ASSERT(-0x10000 <= x && x <= 0x10000);
    if (x == 0) return 0;
    if (x == +0x10000) return +0x10000;
    if (x == -0x10000) return -0x10000;
    u32 i = (u32)abs_i32(x);
    u32 r;
    r = 0x030D;
    r = 0x0C1A - ((r * i) >> 16);
    r = 0x2292 - ((r * i) >> 16);
    r = 0xFFFC - ((r * i) >> 16);
    r = sqrt_u32(((0x10000 - i) << 16) + 0x800) * r;
    r = ((0xFFFF8000U - r)) >> 16;
    return (x > 0 ? +(i32)r : -(i32)r);
}

// INPUT:  [-0x10000,0x10000] = [-1;1]
// OUTPUT: [0, 0x20000] = [0;PI]
static i32 acos_q16(i32 x)
{
    return 0x10000 - asin_q16(x);
}

static inline f32 sin_f(f32 x)
{
    return sinf(x);
}

static inline f32 cos_f(f32 x)
{
    return cosf(x);
}

static inline f32 atan2_f(f32 y, f32 x)
{
    return atan2f(y, x);
}

// ============================================================================
// V2I
// ============================================================================

// returns the minimum component
static inline v2_i32 v2_min(v2_i32 a, v2_i32 b)
{
    v2_i32 r = {min_i32(a.x, b.x), min_i32(a.y, b.y)};
    return r;
}

// returns the minimum component
static inline v2_i32 v2_min3(v2_i32 a, v2_i32 b, v2_i32 c)
{
    v2_i32 r = {min_i32(a.x, min_i32(b.x, c.x)),
                min_i32(a.y, min_i32(b.y, c.y))};
    return r;
}

// returns the maximum component
static inline v2_i32 v2_max(v2_i32 a, v2_i32 b)
{
    v2_i32 r = {max_i32(a.x, b.x), max_i32(a.y, b.y)};
    return r;
}

// returns the minimum component
static inline v2_i32 v2_max3(v2_i32 a, v2_i32 b, v2_i32 c)
{
    v2_i32 r = {max_i32(a.x, max_i32(b.x, c.x)),
                max_i32(a.y, max_i32(b.y, c.y))};
    return r;
}

static inline v2_i32 v2_inv(v2_i32 a)
{
    v2_i32 r = {-a.x, -a.y};
    return r;
}

static inline v2_i8 v2_i8_add(v2_i8 a, v2_i8 b)
{
    v2_i8 r = {a.x + b.x, a.y + b.y};
    return r;
}

static inline v2_i32 v2_add(v2_i32 a, v2_i32 b)
{
    v2_i32 r = {a.x + b.x, a.y + b.y};
    return r;
}

static inline v2_i32 v2_sub(v2_i32 a, v2_i32 b)
{
    v2_i32 r = {a.x - b.x, a.y - b.y};
    return r;
}

static inline v2_i32 v2_shr(v2_i32 a, i32 s)
{
    v2_i32 r = {a.x >> s, a.y >> s};
    return r;
}

static inline v2_i32 v2_shl(v2_i32 a, i32 s)
{
    v2_i32 r = {a.x << s, a.y << s};
    return r;
}

static inline v2_i32 v2_mul(v2_i32 a, i32 s)
{
    v2_i32 r = {a.x * s, a.y * s};
    return r;
}

static inline v2_i32 v2_div(v2_i32 a, i32 s)
{
    v2_i32 r = {a.x / s, a.y / s};
    return r;
}

static inline v2_i32 v2_mulq(v2_i32 a, i32 n, i32 q)
{
    return v2_shr(v2_mul(a, n), q);
}

static inline i32 v2_eq(v2_i32 a, v2_i32 b)
{
    return a.x == b.x && a.y == b.y;
}

static inline i32 v2_dot(v2_i32 a, v2_i32 b)
{
    return a.x * b.x + a.y * b.y;
}

static inline i32 v2_crs(v2_i32 a, v2_i32 b)
{
    return a.x * b.y - a.y * b.x;
}

static inline u32 v2_lensq(v2_i32 a)
{
    u32 x = (u32)abs_i32(a.x);
    u32 y = (u32)abs_i32(a.y);
    u64 v = ((u64)x * x + (u64)y * y);
    assert(v <= (u64)(x * x + y * y));
    return (x * x + y * y);
}

static inline u64 v2_lensql(v2_i32 a)
{
    u64 x = (u64)abs_i32(a.x);
    u64 y = (u64)abs_i32(a.y);
    return (x * x + y * y);
}

static inline f32 v2_lensq_f(v2_i32 v)
{
    return (f32)v.x * (f32)v.x + (f32)v.y * (f32)v.y;
}

static inline i32 v2_len(v2_i32 v)
{
    return sqrt_u32(v2_lensq(v));
}

static inline u32 v2_lenl(v2_i32 v)
{
    return sqrt_u64(v2_lensql(v));
}

static inline f32 v2_lenf(v2_i32 v)
{
    return sqrt_f32(v2_lensq_f(v));
}

static inline u32 v2_distancesq(v2_i32 a, v2_i32 b)
{
    return v2_lensq(v2_sub(a, b));
}

static inline i32 v2_distance(v2_i32 a, v2_i32 b)
{
    return sqrt_u32(v2_distancesq(a, b));
}

static inline v2_i32 v2_setlenl(v2_i32 a, u32 l, u32 len)
{
    if (l <= 2) {
        v2_i32 r0 = {len, 0};
        return r0;
    }

    v2_i32 r = {(i32)(((i64)a.x * (i64)len) / (i64)l),
                (i32)(((i64)a.y * (i64)len) / (i64)l)};
    return r;
}

static inline v2_i32 v2_setlen(v2_i32 a, i32 len)
{
    return v2_setlenl(a, v2_len(a), len);
}

static inline v2_i32 v2_truncate(v2_i32 a, i32 l)
{
    u32 ls = v2_lensq(a);
    if (ls <= (u32)l * (u32)l) return a;
    return v2_setlenl(a, sqrt_u32(ls), l);
}

static v2_i32 v2_lerp(v2_i32 a, v2_i32 b, i32 num, i32 den)
{
    v2_i32 v = {lerp_i32(a.x, b.x, num, den),
                lerp_i32(a.y, b.y, num, den)};
    return v;
}

static v2_i32 v2_lerpl(v2_i32 a, v2_i32 b, i32 num, i32 den)
{
    v2_i32 v = {lerpl_i32(a.x, b.x, num, den),
                lerpl_i32(a.y, b.y, num, den)};
    return v;
}

// ============================================================================
// V2 I16
// ============================================================================

static inline i32 v2_i16_dot(v2_i16 a, v2_i16 b)
{
    return i16x2_muad(i16x2_from_v2_i16(a), i16x2_from_v2_i16(b));
}

static inline i32 v2_i16_crs(v2_i16 a, v2_i16 b)
{
    return i16x2_musdx(i16x2_from_v2_i16(a), i16x2_from_v2_i16(b));
}

static inline i32 v2_i16_lensq(v2_i16 a)
{
    i16x2 x = i16x2_from_v2_i16(a);
    return i16x2_muad(x, x);
}

static inline v2_i16 v2_i16_add(v2_i16 a, v2_i16 b)
{
    i16x2 r = i16x2_add(i16x2_from_v2_i16(a), i16x2_from_v2_i16(b));
    return v2_i16_from_i16x2(r);
}

static inline v2_i16 v2_i16_sub(v2_i16 a, v2_i16 b)
{
    i16x2 r = i16x2_sub(i16x2_from_v2_i16(a), i16x2_from_v2_i16(b));
    return v2_i16_from_i16x2(r);
}

static inline bool32 v2_i16_eq(v2_i16 a, v2_i16 b)
{
    return (a.x == b.x && a.y == b.y);
}

static inline v2_i16 v2_i16_shr(v2_i16 v, i32 s)
{
    i16x2  k = i16x2_from_v2_i16(v);
    v2_i16 r = v2_i16_from_i16x2(i16x2_shr(k, s));
    return r;
}

static inline v2_i16 v2_i16_shl(v2_i16 v, i32 s)
{
    i16x2  k = i16x2_from_v2_i16(v);
    v2_i16 r = v2_i16_from_i16x2(i16x2_shl(k, s));
    return r;
}

static inline i32 v2_i16_distancesq(v2_i16 a, v2_i16 b)
{
    return v2_i16_lensq(v2_i16_sub(a, b));
}

// ============================================================================
// V2 F32
// ============================================================================

static inline v2_f32 v2f_sub(v2_f32 a, v2_f32 b)
{
    v2_f32 r = {a.x - b.x, a.y - b.y};
    return r;
}

static inline v2_f32 v2f_add(v2_f32 a, v2_f32 b)
{
    v2_f32 r = {a.x + b.x, a.y + b.y};
    return r;
}

static inline v2_f32 v2f_mul(v2_f32 a, f32 s)
{
    v2_f32 r = {a.x * s, a.y * s};
    return r;
}

static inline f32 v2f_lensq(v2_f32 a)
{
    return (a.x * a.x + a.y * a.y);
}

static inline f32 v2f_len(v2_f32 a)
{
    return sqrt_f32(v2f_lensq(a));
}

static inline v2_f32 v2f_setlen(v2_f32 a, f32 len)
{
    f32 l = v2f_len(a);
    if (l == 0.f) {
        return (v2_f32){len, 0.f};
    }

    v2_f32 r = {a.x * (len / l), a.y * (len / l)};
    return r;
}

// ============================================================================

static bool32 intersect_rec(rec_i32 a, rec_i32 b, rec_i32 *r)
{
    i32 ax1 = a.x, ax2 = a.x + a.w;
    i32 ay1 = a.y, ay2 = a.y + a.h;
    i32 bx1 = b.x, bx2 = b.x + b.w;
    i32 by1 = b.y, by2 = b.y + b.h;
    i32 rx1 = ax1 > bx1 ? ax1 : bx1; // x1/y1 is top left
    i32 ry1 = ay1 > by1 ? ay1 : by1;
    i32 rx2 = ax2 < bx2 ? ax2 : bx2; // x2/y2 is bot right
    i32 ry2 = ay2 < by2 ? ay2 : by2;
    if (rx2 <= rx1 || ry2 <= ry1) return 0;
    if (r) {
        rec_i32 rec = {rx1, ry1, rx2 - rx1, ry2 - ry1};
        *r          = rec;
    }
    return 1;
}

static inline bool32 overlap_rec_pnt(rec_i32 a, v2_i32 p)
{
    return (a.x <= p.x && p.x < a.x + a.w && a.y <= p.y && p.y < a.y + a.h);
}

// check for overlap - touching rectangles considered overlapped
static inline bool32 overlap_rec_touch(rec_i32 a, rec_i32 b)
{
    return (b.x <= a.x + a.w && a.x <= b.x + b.w &&
            b.y <= a.y + a.h && a.y <= b.y + b.h);
}

// check for overlap - touching rectangles considered NOT overlapped
static inline bool32 overlap_rec(rec_i32 a, rec_i32 b)
{
    return (b.x < a.x + a.w && a.x < b.x + b.w &&
            b.y < a.y + a.h && a.y < b.y + b.h);
}

static inline bool32 overlap_rec_circ(rec_i32 rec, v2_i32 ctr, u32 r)
{
    i32    rx1 = rec.x, rx2 = rec.x + rec.w;
    i32    ry1 = rec.y, ry2 = rec.y + rec.h;
    v2_i32 tp = ctr;

    if (ctr.x < rx1) tp.x = rx1;
    else if (ctr.x > rx2) tp.x = rx2;
    if (ctr.y < ry1) tp.y = ry1;
    else if (ctr.y > ry2) tp.y = ry2;

    return (v2_distancesq(ctr, tp) < r * r);
}

static rec_i32 translate_rec(rec_i32 r, v2_i32 p)
{
    rec_i32 rr = {r.x + p.x, r.y + p.y, r.w, r.h};
    return rr;
}

static tri_i32 translate_tri(tri_i32 t, v2_i32 p)
{
    tri_i32 tt = {{v2_add(t.p[0], p), v2_add(t.p[1], p), v2_add(t.p[2], p)}};
    return tt;
}

// turns an AABB into two triangles
static void tris_from_rec(rec_i32 r, tri_i32 tris[2])
{
    tri_i32 t1 = {{{r.x, r.y},
                   {r.x + r.w, r.y},
                   {r.x + r.w, r.y + r.h}}};
    tri_i32 t2 = {{{r.x, r.y},
                   {r.x + r.w, r.y + r.h},
                   {r.x, r.y + r.h}}};
    tris[0]    = t1;
    tris[1]    = t2;
}

static rec_i32 rec_from_tri(tri_i32 t)
{
    v2_i32  pmin = v2_min(t.p[0], v2_min(t.p[1], t.p[2]));
    v2_i32  pmax = v2_max(t.p[0], v2_max(t.p[1], t.p[2]));
    rec_i32 r    = {pmin.x, pmin.y,
                    pmax.x - pmin.x, pmax.y - pmin.y};
    return r;
}

// 0-----1
// |     |
// 3-----2
static void points_from_rec(rec_i32 r, v2_i32 *pts)
{
    pts[0].x = r.x;
    pts[0].y = r.y;
    pts[1].x = r.x + r.w;
    pts[1].y = r.y;
    pts[2].x = r.x + r.w;
    pts[2].y = r.y + r.h;
    pts[3].x = r.x;
    pts[3].y = r.y + r.h;
}

// returns interpolation data for the intersection:
// S = A + [(B - A) * u] / den;
// S = C + [(D - C) * v] / den;
static inline void intersect_line_uv(v2_i32 a, v2_i32 b, v2_i32 c, v2_i32 d,
                                     i32 *u, i32 *v, i32 *den)
{
    assert(u && v && den);
    v2_i32 x = v2_sub(a, c);
    v2_i32 y = v2_sub(c, d);
    v2_i32 z = v2_sub(a, b);
    *u       = v2_crs(x, y),
    *v       = v2_crs(x, z),
    *den     = v2_crs(z, y);
}

// returns interpolation data for the intersection:
// S = A + [(B - A) * u] / den;
static inline void intersect_line_u(v2_i32 a, v2_i32 b, v2_i32 c, v2_i32 d,
                                    i32 *u, i32 *den)
{
    assert(u && den);
    v2_i32 x = v2_sub(a, c);
    v2_i32 y = v2_sub(c, d);
    v2_i32 z = v2_sub(a, b);
    *u       = v2_crs(x, y),
    *den     = v2_crs(z, y);
}

/* check if point on linesegment - on endpoints considered NOT overlapped
 * px = x0 + (x1 - x0) * s
 * py = y0 + (y1 - y0) * s
 *
 * s = (px - x0) / (x1 - x0)
 * s = (py - y0) / (y1 - y0)
 *
 * s has to be between 0 and 1 for p to be on the line segment
 * also cross product == 0
 */
static bool32 overlap_lineseg_pnt_excl(lineseg_i32 l, v2_i32 p)
{
    v2_i32 d1 = v2_sub(p, l.a);
    v2_i32 d2 = v2_sub(l.b, l.a);
    return (v2_crs(d1, d2) == 0 &&
            (between_excl_i32(d1.x, 0, d2.x) || //  <-- why here OR...?
             between_excl_i32(d1.y, 0, d2.y)));
}

// check if point on linesegment - on endpoints considered overlapped
static bool32 overlap_lineseg_pnt_incl(lineseg_i32 l, v2_i32 p)
{
    v2_i32 d1 = v2_sub(p, l.a);
    v2_i32 d2 = v2_sub(l.b, l.a);
    return (v2_crs(d1, d2) == 0 &&
            (between_incl_i32(d1.x, 0, d2.x) && //  <-- ...and here AND?
             between_incl_i32(d1.y, 0, d2.y)));
}

// check if point on linesegment - on endpoints considered overlapped
static bool32 overlap_lineseg_pnt_incl_excl(lineseg_i32 l, v2_i32 p)
{
    v2_i32 d1 = v2_sub(p, l.a);
    v2_i32 d2 = v2_sub(l.b, l.a);
    return (v2_crs(d1, d2) == 0 &&
            (between_incl_excl_i32(d1.x, 0, d2.x) || //  <-- ...and here AND?
             between_incl_excl_i32(d1.y, 0, d2.y)));
}

// check if point on linesegment - on endpoints considered NOT overlapped
static inline bool32 overlap_line_pnt(line_i32 l, v2_i32 p)
{
    return (v2_crs(v2_sub(l.b, l.a), v2_sub(p, l.a)) == 0);
}

// check for overlap - touching endpoints considered overlapped
static bool32 overlap_lineseg_incl(lineseg_i32 la, lineseg_i32 lb)
{
    i32 u, v, d;
    intersect_line_uv(la.a, la.b, lb.a, lb.b, &u, &v, &d);
    return (((0 <= u && u <= d) || (d <= u && u <= 0)) && // la
            ((0 <= v && v <= d) || (d <= v && v <= 0)));  // lb
}

// check for overlap - touching endpoints considered NOT overlapped
static bool32 overlap_lineseg_excl(lineseg_i32 la, lineseg_i32 lb)
{
    i32 u, v, d;
    intersect_line_uv(la.a, la.b, lb.a, lb.b, &u, &v, &d);
    return (((0 < u && u < d) || (d < u && u < 0)) && // la
            ((0 < v && v < d) || (d < v && v < 0)));  // lb
}

// check for overlap - touching endpoints considered NOT overlapped
static bool32 overlap_lineseg_excl_onesided(lineseg_i32 la, lineseg_i32 lb)
{
    i32 u, v, d;
    intersect_line_uv(la.a, la.b, lb.a, lb.b, &u, &v, &d);
    return (((0 <= u && u <= d) || (d <= u && u <= 0)) && // la
            ((0 <= v && v < d) || (d < v && v <= 0)));    // lb
}

// check for overlap - touching endpoints considered overlapped
static bool32 overlap_lineseg_lineray_incl(lineseg_i32 la, lineray_i32 lb)
{
    i32 u, v, d;
    intersect_line_uv(la.a, la.b, lb.a, lb.b, &u, &v, &d);
    return (((0 <= u && u <= d) || (d <= u && u <= 0)) && // la
            ((0 <= v && 0 <= d) || (v <= 0 && d <= 0)));  // v/w [0, inf]
}

// check for overlap - touching endpoints considered NOT overlapped
static bool32 overlap_lineseg_lineray_excl(lineseg_i32 la, lineray_i32 lb)
{
    i32 u, v, d;
    intersect_line_uv(la.a, la.b, lb.a, lb.b, &u, &v, &d);
    return (((0 < u && u < d) || (d < u && u < 0)) && // la
            ((0 < v && 0 < d) || (v < 0 && d < 0)));  // v/w [0, inf]
}

// check for overlap - touching endpoints considered overlapped
static bool32 overlap_lineseg_line_incl(lineseg_i32 la, line_i32 lb)
{
    i32 u, d;
    intersect_line_u(la.a, la.b, lb.a, lb.b, &u, &d);
    return ((0 <= u && u <= d) || (d <= u && u <= 0)); // la
}

// check for overlap - touching endpoints considered NOT overlapped
static bool32 overlap_lineseg_line_excl(lineseg_i32 la, line_i32 lb)
{
    i32 u, d;
    intersect_line_u(la.a, la.b, lb.a, lb.b, &u, &d);
    return ((0 < u && u < d) || (d < u && u < 0)); // la
}

// projects a point onto a line and clamps it to the endpoints if not infinite
// S = A + [(B - A) * u] / den
static inline ratio_s project_pnt_line_ratio(v2_i32 p, v2_i32 a, v2_i32 b)
{
    v2_i32 pnt = v2_sub(p, a);
    v2_i32 dir = v2_sub(b, a);
    u32    d   = v2_lensq(dir);
    i32    s   = v2_dot(dir, pnt);
    if (d > I32_MAX) { // reduce precision to fit both into i32
        d >>= 1;
        s >>= 1;
    }
    ratio_s r = {s, (i32)d};
    return r;
}

static v2_i32 project_pnt_line(v2_i32 p, v2_i32 a, v2_i32 b)
{
    ratio_s r = project_pnt_line_ratio(p, a, b);
    v2_i32  t = v2_lerpl(a, b, r.num, r.den);
    return t;
}

static inline void barycentric_uvw(v2_i32 a, v2_i32 b, v2_i32 c, v2_i32 p,
                                   i32 *u, i32 *v, i32 *w)
{
    *u = v2_crs(v2_sub(b, a), v2_sub(p, a));
    *v = v2_crs(v2_sub(a, c), v2_sub(p, c));
    *w = v2_crs(v2_sub(c, b), v2_sub(p, b));
}

static inline void tri_pnt_barycentric_uvw(tri_i32 t, v2_i32 p,
                                           i32 *u, i32 *v, i32 *w)
{
    barycentric_uvw(t.p[0], t.p[1], t.p[2], p, u, v, w);
}

// check for overlap - touching tri considered NOT overlapped
static inline bool32 overlap_tri_pnt_excl(tri_i32 t, v2_i32 p)
{
    i32 u, v, w;
    tri_pnt_barycentric_uvw(t, p, &u, &v, &w);
    bool32 a = (u > 0 && v > 0 && w > 0) || (u < 0 && v < 0 && w < 0);
    bool32 b = (u > 0 && v > 0 && w > 0) || (u & v & w) < 0; // not entirely sure
    assert(a == b);
    return a;
}

// check for overlap - touching tri considered overlapped
static inline bool32 overlap_tri_pnt_incl2(tri_i32 t, v2_i32 p)
{
    i32 u, v, w;
    tri_pnt_barycentric_uvw(t, p, &u, &v, &w);
    return (u | v | w) >= 0 || (u <= 0 && v <= 0 && w <= 0);
}

// check for overlap - touching tri considered overlapped
static inline bool32 overlap_tri_pnt_incl(tri_i32 t, v2_i32 p)
{
    i32 u = v2_crs(v2_sub(t.p[1], t.p[0]), v2_sub(p, t.p[0]));
    i32 v = v2_crs(v2_sub(t.p[0], t.p[2]), v2_sub(p, t.p[2]));
    i32 w = v2_crs(v2_sub(t.p[2], t.p[1]), v2_sub(p, t.p[1]));
    return (u | v | w) >= 0 || (u <= 0 && v <= 0 && w <= 0);
}

static inline bool32 overlap_tri_pnt_incl_i16(tri_i16 t, v2_i16 p)
{
    i32 u = v2_i16_crs(v2_i16_sub(t.p[1], t.p[0]), v2_i16_sub(p, t.p[0]));
    i32 v = v2_i16_crs(v2_i16_sub(t.p[0], t.p[2]), v2_i16_sub(p, t.p[2]));
    i32 w = v2_i16_crs(v2_i16_sub(t.p[2], t.p[1]), v2_i16_sub(p, t.p[1]));
    return (u | v | w) >= 0 || (u <= 0 && v <= 0 && w <= 0);
}

/* separating axis check
 * check all lines if all other points of the triangle are on
 * one side and all other points of the line are on the other side
 * of the axis
 *
 * doesn't handle degenerate triangles (I think)
 */
static bool32 overlap_tri_lineseg_excl(tri_i32 t, lineseg_i32 l)
{
    v2_i32 w = v2_sub(l.b, l.a);
    v2_i32 x = v2_sub(t.p[1], t.p[0]);
    v2_i32 y = v2_sub(t.p[2], t.p[0]);
    v2_i32 z = v2_sub(t.p[2], t.p[1]);
    v2_i32 a = v2_sub(l.a, t.p[0]);
    v2_i32 b = v2_sub(l.b, t.p[0]);
    v2_i32 c = v2_sub(l.a, t.p[1]);
    v2_i32 d = v2_sub(l.a, t.p[2]);
    v2_i32 e = v2_sub(l.b, t.p[1]);

    i32 a0 = v2_crs(x, y);
    i32 a1 = v2_crs(a, x);
    i32 a2 = v2_crs(b, x);
    i32 b1 = v2_crs(a, y);
    i32 b2 = v2_crs(b, y);
    i32 c0 = v2_crs(x, z);
    i32 c1 = v2_crs(c, z);
    i32 c2 = v2_crs(e, z);
    i32 d0 = v2_crs(w, a);
    i32 d1 = v2_crs(w, c);
    i32 d2 = v2_crs(w, d);
    i32 b0 = -a0;

    bool32 separated =
        (a0 | a1 | a2) >= 0 || (a0 <= 0 && a1 <= 0 && a2 <= 0) ||
        (b0 | b1 | b2) >= 0 || (b0 <= 0 && b1 <= 0 && b2 <= 0) ||
        (c0 | c1 | c2) >= 0 || (c0 <= 0 && c1 <= 0 && c2 <= 0) ||
        (d0 | d1 | d2) >= 0 || (d0 <= 0 && d1 <= 0 && d2 <= 0);
    return !separated;
}

static bool32 overlap_tri_lineseg_excl_i16(tri_i16 t, lineseg_i16 l)
{
    v2_i16 w = v2_i16_sub(l.b, l.a);
    v2_i16 x = v2_i16_sub(t.p[1], t.p[0]);
    v2_i16 y = v2_i16_sub(t.p[2], t.p[0]);
    v2_i16 z = v2_i16_sub(t.p[2], t.p[1]);
    v2_i16 a = v2_i16_sub(l.a, t.p[0]);
    v2_i16 b = v2_i16_sub(l.b, t.p[0]);
    v2_i16 c = v2_i16_sub(l.a, t.p[1]);
    v2_i16 d = v2_i16_sub(l.a, t.p[2]);
    v2_i16 e = v2_i16_sub(l.b, t.p[1]);

    i32 a0 = v2_i16_crs(x, y);
    i32 a1 = v2_i16_crs(a, x);
    i32 a2 = v2_i16_crs(b, x);
    i32 b1 = v2_i16_crs(a, y);
    i32 b2 = v2_i16_crs(b, y);
    i32 c0 = v2_i16_crs(x, z);
    i32 c1 = v2_i16_crs(c, z);
    i32 c2 = v2_i16_crs(e, z);
    i32 d0 = v2_i16_crs(w, a);
    i32 d1 = v2_i16_crs(w, c);
    i32 d2 = v2_i16_crs(w, d);
    i32 b0 = -a0;

    bool32 separated =
        (a0 | a1 | a2) >= 0 || (a0 <= 0 && a1 <= 0 && a2 <= 0) ||
        (b0 | b1 | b2) >= 0 || (b0 <= 0 && b1 <= 0 && b2 <= 0) ||
        (c0 | c1 | c2) >= 0 || (c0 <= 0 && c1 <= 0 && c2 <= 0) ||
        (d0 | d1 | d2) >= 0 || (d0 <= 0 && d1 <= 0 && d2 <= 0);
    return !separated;
}

// backup overlap
static bool32 overlap_tri_excl_backup(tri_i32 tri1, tri_i32 tri2)
{
    v2_i32 p0 = tri1.p[0], p1 = tri1.p[1], p2 = tri1.p[2];
    v2_i32 pa = tri2.p[0], pb = tri2.p[1], pc = tri2.p[2];
    v2_i32 v0 = v2_min(p0, v2_min(p1, p2));
    v2_i32 v1 = v2_max(p0, v2_max(p1, p2));
    v2_i32 va = v2_min(pa, v2_min(pb, pc));
    v2_i32 vb = v2_max(pa, v2_max(pb, pc));
    if (v0.x >= vb.x || v0.y >= vb.y || va.x >= v1.x || va.y >= v1.y)
        return 0;

    v2_i32      pts1[4] = {p0, p1, p2, p0};
    lineseg_i32 lsa     = {pa, pb};
    lineseg_i32 lsb     = {pb, pc};
    lineseg_i32 lsc     = {pa, pc};
    for (i32 n = 0; n < 3; n++) {
        lineseg_i32 lls = {pts1[n], pts1[n + 1]};

        if (overlap_lineseg_excl(lls, lsa) ||
            overlap_lineseg_excl(lls, lsb) ||
            overlap_lineseg_excl(lls, lsc) ||
            overlap_tri_pnt_excl(tri1, tri2.p[n]) ||
            overlap_tri_pnt_excl(tri2, tri1.p[n]))
            return 1;
    }
    return 0;
}

static bool32 overlap_rec_lineseg_excl(rec_i32 r, lineseg_i32 l)
{
    v2_i32 p[4];
    points_from_rec(r, p);
    if ((l.a.x <= p[0].x && l.b.x <= p[0].x) ||
        (l.a.y <= p[0].y && l.b.y <= p[0].y) ||
        (l.a.x >= p[2].x && l.b.x >= p[2].x) ||
        (l.a.y >= p[2].y && l.b.y >= p[2].y))
        return 0;

    v2_i32 dt        = v2_sub(l.b, l.a);
    i32    a0        = v2_crs(v2_sub(p[0], l.a), dt);
    i32    a1        = v2_crs(v2_sub(p[1], l.a), dt);
    i32    a2        = v2_crs(v2_sub(p[2], l.a), dt);
    i32    a3        = v2_crs(v2_sub(p[3], l.a), dt);
    bool32 separated = (a0 | a1 | a2 | a3) >= 0 ||
                       (a0 <= 0 && a1 <= 0 && a2 <= 0 && a3 <= 0);
    return !separated;
}

static m33_f32 m33_identity()
{
    m33_f32 m = {{1.f, 0.f, 0.f,
                  0.f, 1.f, 0.f,
                  0.f, 0.f, 1.f}};
    return m;
}

static m33_f32 m33_add(m33_f32 a, m33_f32 b)
{
    m33_f32 m;
    for (i32 n = 0; n < 9; n++)
        m.m[n] = a.m[n] + b.m[n];
    return m;
}

static m33_f32 m33_sub(m33_f32 a, m33_f32 b)
{
    m33_f32 m;
    for (i32 n = 0; n < 9; n++)
        m.m[n] = a.m[n] - b.m[n];
    return m;
}

static m33_f32 m33_mul(m33_f32 a, m33_f32 b)
{
    m33_f32 m;
    for (i32 i = 0; i < 3; i++) {
        for (i32 j = 0; j < 3; j++) {
            m.m[i + j * 3] = a.m[i + 0 * 3] * b.m[0 + j * 3] +
                             a.m[i + 1 * 3] * b.m[1 + j * 3] +
                             a.m[i + 2 * 3] * b.m[2 + j * 3];
        }
    }
    return m;
}

static m33_f32 m33_rotate(f32 angle)
{
    f32     si = sinf(angle);
    f32     co = cosf(angle);
    m33_f32 m  = {{+co, -si, 0.f,
                   +si, +co, 0.f,
                   0.f, 0.f, 1.f}};
    return m;
}

static m33_f32 m33_scale(f32 scx, f32 scy)
{
    m33_f32 m = {{scx, 0.f, 0.f,
                  0.f, scy, 0.f,
                  0.f, 0.f, 1.f}};
    return m;
}

static m33_f32 m33_shear(f32 shx, f32 shy)
{
    m33_f32 m = {{1.f, shx, 0.f,
                  shy, 1.f, 0.f,
                  0.f, 0.f, 1.f}};
    return m;
}

static m33_f32 m33_offset(f32 x, f32 y)
{
    m33_f32 m = {{1.f, 0.f, x,
                  0.f, 1.f, y,
                  0.f, 0.f, 1.f}};
    return m;
}

static v2_f32 v2f_lerp(v2_f32 a, v2_f32 b, f32 t)
{
    v2_f32 r = {
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t};
    return r;
}

static v2_f32 v2_spline(v2_f32 x, v2_f32 y, v2_f32 x_tang, v2_f32 y_tang, f32 t)
{
    v2_f32 u = v2f_add(x, x_tang);
    v2_f32 v = v2f_add(y, y_tang);
    v2_f32 a = v2f_lerp(x, u, t);
    v2_f32 b = v2f_lerp(u, v, t);
    v2_f32 c = v2f_lerp(v, y, t);
    v2_f32 d = v2f_lerp(a, b, t);
    v2_f32 e = v2f_lerp(b, c, t);
    v2_f32 f = v2f_lerp(d, e, t);
    return f;
}

static const i32 cos_table[256] = {
    0x10000, 0x0FFFF, 0x0FFFB, 0x0FFF5, 0x0FFEC, 0x0FFE1, 0x0FFD4, 0x0FFC4,
    0x0FFB1, 0x0FF9C, 0x0FF85, 0x0FF6B, 0x0FF4E, 0x0FF30, 0x0FF0E, 0x0FEEB,
    0x0FEC4, 0x0FE9C, 0x0FE71, 0x0FE43, 0x0FE13, 0x0FDE1, 0x0FDAC, 0x0FD74,
    0x0FD3B, 0x0FCFE, 0x0FCC0, 0x0FC7F, 0x0FC3B, 0x0FBF5, 0x0FBAD, 0x0FB62,
    0x0FB15, 0x0FAC5, 0x0FA73, 0x0FA1F, 0x0F9C8, 0x0F96E, 0x0F913, 0x0F8B4,
    0x0F854, 0x0F7F1, 0x0F78C, 0x0F724, 0x0F6BA, 0x0F64E, 0x0F5DF, 0x0F56E,
    0x0F4FA, 0x0F484, 0x0F40C, 0x0F391, 0x0F314, 0x0F295, 0x0F213, 0x0F18F,
    0x0F109, 0x0F080, 0x0EFF5, 0x0EF68, 0x0EED9, 0x0EE47, 0x0EDB3, 0x0ED1C,
    0x0EC83, 0x0EBE8, 0x0EB4B, 0x0EAAB, 0x0EA0A, 0x0E966, 0x0E8BF, 0x0E817,
    0x0E76C, 0x0E6BF, 0x0E60F, 0x0E55E, 0x0E4AA, 0x0E3F4, 0x0E33C, 0x0E282,
    0x0E1C6, 0x0E107, 0x0E046, 0x0DF83, 0x0DEBE, 0x0DDF7, 0x0DD2D, 0x0DC62,
    0x0DB94, 0x0DAC4, 0x0D9F2, 0x0D91E, 0x0D848, 0x0D770, 0x0D696, 0x0D5B9,
    0x0D4DB, 0x0D3FB, 0x0D318, 0x0D234, 0x0D14D, 0x0D065, 0x0CF7A, 0x0CE8D,
    0x0CD9F, 0x0CCAE, 0x0CBBC, 0x0CAC7, 0x0C9D1, 0x0C8D9, 0x0C7DE, 0x0C6E2,
    0x0C5E4, 0x0C4E4, 0x0C3E2, 0x0C2DE, 0x0C1D8, 0x0C0D1, 0x0BFC7, 0x0BEBC,
    0x0BDAF, 0x0BCA0, 0x0BB8F, 0x0BA7D, 0x0B968, 0x0B852, 0x0B73A, 0x0B620,
    0x0B505, 0x0B3E8, 0x0B2C9, 0x0B1A8, 0x0B086, 0x0AF61, 0x0AE3C, 0x0AD14,
    0x0ABEB, 0x0AAC0, 0x0A994, 0x0A866, 0x0A736, 0x0A605, 0x0A4D2, 0x0A39D,
    0x0A267, 0x0A130, 0x09FF7, 0x09EBC, 0x09D80, 0x09C42, 0x09B03, 0x099C2,
    0x09880, 0x0973C, 0x095F7, 0x094B0, 0x09368, 0x0921E, 0x090D4, 0x08F87,
    0x08E3A, 0x08CEB, 0x08B9A, 0x08A48, 0x088F5, 0x087A1, 0x0864B, 0x084F4,
    0x0839C, 0x08242, 0x080E8, 0x07F8C, 0x07E2E, 0x07CD0, 0x07B70, 0x07A0F,
    0x078AD, 0x0774A, 0x075E6, 0x07480, 0x07319, 0x071B2, 0x07049, 0x06EDF,
    0x06D74, 0x06C08, 0x06A9B, 0x0692D, 0x067BE, 0x0664D, 0x064DC, 0x0636A,
    0x061F7, 0x06083, 0x05F0E, 0x05D98, 0x05C22, 0x05AAA, 0x05932, 0x057B8,
    0x0563E, 0x054C3, 0x05347, 0x051CB, 0x0504D, 0x04ECF, 0x04D50, 0x04BD0,
    0x04A50, 0x048CF, 0x0474D, 0x045CA, 0x04447, 0x042C3, 0x0413F, 0x03FB9,
    0x03E34, 0x03CAD, 0x03B26, 0x0399F, 0x03817, 0x0368E, 0x03505, 0x0337B,
    0x031F1, 0x03066, 0x02EDB, 0x02D50, 0x02BC4, 0x02A37, 0x028AB, 0x0271D,
    0x02590, 0x02402, 0x02273, 0x020E5, 0x01F56, 0x01DC7, 0x01C37, 0x01AA7,
    0x01917, 0x01787, 0x015F6, 0x01466, 0x012D5, 0x01144, 0x00FB2, 0x00E21,
    0x00C8F, 0x00AFE, 0x0096C, 0x007DA, 0x00648, 0x004B6, 0x00324, 0x00192};

#endif