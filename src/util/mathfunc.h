// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef MATHFUNC_H
#define MATHFUNC_H

#include "pltf/pltf_intrin.h"

#define PI_FLOAT      3.1415927f
#define PI2_FLOAT     6.2831853f
#define PI2_FLOAT_INV 0.15915494f

#define QX_FRAC(Q, D, N) (((D) << (Q)) / (N))
#define Q8_FRAC(D, N)    QX_FRAC(8, D, N)
#define Q12_FRAC(D, N)   QX_FRAC(12, D, N)
#define Q16_FRAC(D, N)   QX_FRAC(16, D, N)

typedef i32 (*ease_i32)(i32 a, i32 b, i32 num, i32 den);

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
    ALIGNAS(16)
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
    ALIGNAS(16)
    v2_i32 a;
    v2_i32 b;
} lineseg_i32;

typedef struct { // A-----B--- ray
    ALIGNAS(16)
    v2_i32 a; //
    v2_i32 b; // inf
} lineray_i32;

typedef struct { // ---A-----B--- line
    ALIGNAS(16)
    v2_i32 a; // inf
    v2_i32 b; // inf
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

static inline i32 clamp_i32(i32 x, i32 lo, i32 hi)
{
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

static inline i32 clamp_sym_i32(i32 x, i32 lohi)
{
    return clamp_i32(x, 0 < lohi ? -lohi : +lohi, 0 < lohi ? +lohi : -lohi);
}

static inline i64 clamp_i64(i64 x, i64 lo, i64 hi)
{
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

static inline u32 min3_u32(u32 a, u32 b, u32 c)
{
    return min_u32(a, min_u32(b, c));
}

static inline f32 min_f32(f32 a, f32 b)
{
    return (a < b ? a : b);
}

static inline i32 max_i32(i32 a, i32 b)
{
    return (a > b ? a : b);
}

static inline u32 max_u32(u32 a, u32 b)
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

static inline i64 abs_i64(i64 a)
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

// treats positive and negative numbers the same
static inline i32 shr_balanced_i32(i32 v, i32 s)
{
    return (0 <= v ? v >> s : -((-v) >> s));
}

static i32 i32_range(i32 v, i32 lo, i32 hi)
{
    return lo + (v % (hi - lo + 1));
}

#define SAT_ADD_SUB_IMPL(T, MINV, MAXV)           \
    static inline i32 T##_adds(T a, i32 b)        \
    {                                             \
        return clamp_i32((i32)a + b, MINV, MAXV); \
    }                                             \
                                                  \
    static inline i32 T##_subs(T a, i32 b)        \
    {                                             \
        return clamp_i32((i32)a - b, MINV, MAXV); \
    }

SAT_ADD_SUB_IMPL(u8, 0, U8_MAX)
SAT_ADD_SUB_IMPL(i8, I8_MIN, I8_MAX)
SAT_ADD_SUB_IMPL(u16, 0, U16_MAX)

static inline i32 i32_add_checked(i32 a, i32 b)
{
    i64 r = (i64)a + (i64)b;
    assert(I32_MIN <= r && r <= I32_MAX);
    return (i32)r;
}

static inline i32 i32_sub_checked(i32 a, i32 b)
{
    i64 r = (i64)a - (i64)b;
    assert(I32_MIN <= r && r <= I32_MAX);
    return (i32)r;
}

static inline i32 i32_mul_checked(i32 a, i32 b)
{
    i64 r = (i64)a * (i64)b;
    assert(I32_MIN <= r && r <= I32_MAX);
    return (i32)r;
}

static inline u32 u32_add_checked(u32 a, u32 b)
{
    i64 r = (i64)a + (i64)b;
    assert(0 <= r && r <= U32_MAX);
    return (u32)r;
}

static inline u32 u32_sub_checked(u32 a, u32 b)
{
    i64 r = (i64)a - (i64)b;
    assert(U32_MIN <= r && r <= U32_MAX);
    return (u32)r;
}

static inline u32 u32_mul_checked(u32 a, u32 b)
{
    i64 r = (i64)a * (i64)b;
    assert(U32_MIN <= r && r <= U32_MAX);
    return (u32)r;
}

#define MATHFUNC_USE_OVERFLOW_CHECK PLTF_DEBUG

#if MATHFUNC_USE_OVERFLOW_CHECK
#define i32_add(A, B) i32_add_checked(A, B)
#define i32_sub(A, B) i32_sub_checked(A, B)
#define i32_mul(A, B) i32_mul_checked(A, B)
#define u32_add(A, B) u32_add_checked(A, B)
#define u32_sub(A, B) u32_sub_checked(A, B)
#define u32_mul(A, B) u32_mul_checked(A, B)
#else
#define i32_add(A, B) ((i32)(A) + (i32)(B))
#define i32_sub(A, B) ((i32)(A) - (i32)(B))
#define i32_mul(A, B) ((i32)(A) * (i32)(B))
#define u32_add(A, B) ((u32)(A) + (u32)(B))
#define u32_sub(A, B) ((u32)(A) - (u32)(B))
#define u32_mul(A, B) ((u32)(A) * (u32)(B))
#endif

static inline i32 i32_adds(i32 a, i32 b)
{
    return (i32)clamp_i64((i64)a + b, (i64)I32_MIN, (i64)I32_MAX);
}

static inline i32 i32_subs(i32 a, i32 b)
{
    return (i32)clamp_i64((i64)a - b, (i64)I32_MIN, (i64)I32_MAX);
}

// v % m but result is always positive
static inline i32 modu_i32(i32 v, i32 m)
{
    i32 r = v % m;
    return (0 <= r ? r : r + m);
}

static inline i32 mul_ratio(i32 x, ratio_s r)
{
    return ((x * r.num) / r.den);
}

static inline i32 ratio_i32_mul(i32 x, ratio_i32 r)
{
    return ((x * r.num) / r.den);
}

static inline u32 pow2_u32(u32 v)
{
    return u32_mul(v, v);
}

static inline i32 pow2_i32(i32 v)
{
    return i32_mul(v, v);
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
        r = i32_mul(r, v);
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

// https://en.wikipedia.org/wiki/Alpha_max_plus_beta_min_algorithm
// fast approximation of sqrt(x^2 + y^2)
static i32 amax_bmin_i32(i32 x, i32 y)
{
    i32 xa = abs_i32(x);
    i32 ya = abs_i32(y);
    u32 lo = (u32)min_i32(xa, ya);
    u32 hi = (u32)max_i32(xa, ya);
    // a0 = 1
    // b0 = 0
    // a1 = 29/32
    // b1 = 61/128
    u32 z  = u32_add(u32_mul(hi, 29) >> 5, u32_mul(lo, 61) >> 7);
    return (i32)max_u32(hi, z);
}

// https://en.wikipedia.org/wiki/Alpha_max_plus_beta_min_algorithm
// fast approximation of sqrt(x^2 + y^2)
static f32 amax_bmin_f32(f32 x, f32 y)
{
    f32 xa = abs_f32(x);
    f32 ya = abs_f32(y);
    f32 lo = min_f32(xa, ya);
    f32 hi = max_f32(xa, ya);
    f32 z  = hi * 0.8982042f + lo * 0.4859682f;
    return max_f32(hi, z);
}

// convert fixed point numbers without rounding
static inline i32 q_convert_i32(i32 v, i32 qfrom, i32 qto)
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
    i32 d = abs_i32(den);
    i32 z = i32_mul(i32_sub(b, a), abs_i32(num));
    return (a + (z + (0 < z ? (d >> 1) : -(d >> 1))) / d);
}

static inline i64 lerp_i64(i64 a, i64 b, i64 num, i64 den)
{
    i64 d = abs_i64(den);
    i64 z = (b - a) * abs_i64(num);
    return (a + (z + (0 < z ? (d >> 1) : -(d >> 1))) / d);
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
    if (U64_C(0xFFFFFFFE00000001) <= x) return U32_C(0xFFFFFFFF);
    u64 r = (u64)sqrt_f32((f32)x);
    return (u32)((x < r * r) ? r - 1 : r);
}

static u32 sqrt_u32(u32 x)
{
    if (4294836225 <= x) return 65535;
    u32 r = (u32)(sqrt_f32((f32)x));
    return (x < r * r ? r - 1 : r);
}

#define sqrt_i32 sqrt_u32

static u32 sqrt_u32_bitwise(u32 n)
{
    u32 d = 1 << 30;
    u32 x = n;
    u32 c = 0;

    while (n < d) {
        d >>= 2;
    }

    while (d) {
        u32 t = c + d;
        c >>= 1;

        if (x >= t) {
            x -= t;
            c += d;
        }
        d >>= 2;
    }
    return c;
}

#define Q16_ANGLE_TURN 0x40000

static i32 turn_q18_calc(i32 num, i32 den)
{
    return (0x40000 * num) / den;
}

// maps first quarter of cos to
// [0; PI/2)  -> [0; +1]
// [0; 32768) -> [0; +32768]
static inline i32 cos_quarter_q15(i32 x)
{
    // polynomial: 1 + x2 * (a + x2 * b)
    i32 r = 29463;                 // b
    i32 k = smulwbs(x << 1, x);    // x^2 in q15
    r     = smlawbs(r, k, -80250); // a
    r     = smlawbs(r, k, +32768); // 1 in q15
    return r;
}

// maps first quarter of cos to
// [0; 2*PI)   -> [-1; +1]
// [0; 131072) -> [-32768; +32768]
static i32 cos_q15(i32 x)
{
    i32 i = x & 0x1FFFF; // put x inside [0; 2*PI) = [0; 131072)
    i32 s = 0;
    switch (i >> 15) { // calculate "quadrant"
    case 0: s = +1; break;
    case 1: s = -1, i = 65535 - i; break;
    case 2: s = -1, i = i - 65536; break;
    case 3: s = +1, i = 131071 - i; break;
    }
    return (s * cos_quarter_q15(i));
}

// maps first quarter of cos to
// [0; 2*PI)   -> [-1; +1]
// [0; 131072) -> [-32768; +32768]
static inline i32 sin_q15(i32 x)
{
    return cos_q15(x - 32768);
}

// p: angle/turn, where 2 PI or 1 turn = 262144 (0x40000)
// output: [-65536, 65536] = [-1; +1]
static inline i32 cos_q16(i32 turn_q18)
{
    return (cos_q15(turn_q18 >> 1) << 1);
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
    return (cos_q15(turn_q10 << 7) >> 9);
}

// p: angle, where 1024 = 0x400 = 2 PI
// output: [-64, 64] = [-1; +1]
static inline i32 sin_q6(i32 turn_q10)
{
    return cos_q6(turn_q10 - 0x100);
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
static inline i32 acos_q16(i32 x)
{
    return 0x10000 - asin_q16(x);
}

static f32 cos_f(f32 x)
{
    i32 y = (i32)(fmodf(x * PI2_FLOAT_INV, 1.f) * 131072.f);
    return ((f32)cos_q15(y) / 32768.f);
}

static inline f32 sin_f(f32 x)
{
    return cos_f(x - PI_FLOAT * .5f);
}

// https://dspguru.com/dsp/tricks/fixed-point-atan2-with-self-normalization/
// what is this magic?
static f32 atan2_f(f32 y, f32 x)
{
    // 0.7853982f = 1/4 pi
    // 2.3561945f = 3/4 pi
    f32 a;
    f32 ya = abs_f32(y) + 0.0000001f; // prevent div 0
    if (0 <= x) {
        a = 0.7853982f - 0.7853982f * ((x - ya) / (x + ya));
    } else {
        a = 2.3561945f - 0.7853982f * ((x + ya) / (ya - x));
    }

    return (0 <= y ? +a : -a);
}

// returns angle [-PI; +PI] = [-131072; +131072]
// https://dspguru.com/dsp/tricks/fixed-point-atan2-with-self-normalization/
// what is this magic?
static i32 atan2_i32(i32 y, i32 x)
{
#define ATAN2_1_4_PI 32768 // PI/4
#define ATAN2_1_2_PI 65536 // PI/2

    i64 a;
    i32 ya = abs_i32(y);

    if (0 <= x) {
        i32 d = max_i32(ya + x, 1);
        a     = (i64)(1 * ATAN2_1_4_PI) - (ATAN2_1_4_PI * (i64)(x - ya)) / (i64)d;
    } else {
        i32 d = max_i32(ya - x, 1);
        a     = (i64)(3 * ATAN2_1_4_PI) - (ATAN2_1_4_PI * (i64)(x + ya)) / (i64)d;
    }
    assert(I32_MIN <= a && a <= I32_MAX);
    return (0 <= y ? +a : -a);
}

static i32 atan2_index(f32 y, f32 x, i32 range, i32 adder)
{
    i32 a = (i32)((atan2_f(y, x) * (f32)range) * PI2_FLOAT_INV); // div 2pi
    return (a + range + adder) % range;
}

static i32 atan2_index_pow2(f32 y, f32 x, i32 range, i32 adder)
{
    i32 a = (i32)((atan2_f(y, x) * (f32)range) * PI2_FLOAT_INV); // div 2pi
    return (a + range + adder) & (range - 1);
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
static inline v2_i16 v2_min_i16(v2_i16 a, v2_i16 b)
{
    v2_i16 r = {min_i32(a.x, b.x), min_i32(a.y, b.y)};
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
static inline v2_i16 v2_max_i16(v2_i16 a, v2_i16 b)
{
    v2_i16 r = {max_i32(a.x, b.x), max_i32(a.y, b.y)};
    return r;
}
// returns the minimum component
static inline v2_i32 v2_max3(v2_i32 a, v2_i32 b, v2_i32 c)
{
    v2_i32 r = {max_i32(a.x, max_i32(b.x, c.x)),
                max_i32(a.y, max_i32(b.y, c.y))};
    return r;
}

static inline v2_i32 v2_i32_inv(v2_i32 a)
{
    v2_i32 r = {-a.x, -a.y};
    return r;
}

static inline v2_i8 v2_i8_add(v2_i8 a, v2_i8 b)
{
    v2_i8 r = {a.x + b.x, a.y + b.y};
    return r;
}

static inline v2_i32 v2_i32_add(v2_i32 a, v2_i32 b)
{
    v2_i32 r = {a.x + b.x, a.y + b.y};
    return r;
}

static inline v2_i32 v2_i32_sub(v2_i32 a, v2_i32 b)
{
    v2_i32 r = {a.x - b.x, a.y - b.y};
    return r;
}

static inline v2_i32 v2_i32_shr(v2_i32 a, i32 s)
{
    v2_i32 r = {a.x >> s, a.y >> s};
    return r;
}

static inline v2_i32 v2_i32_shl(v2_i32 a, i32 s)
{
    i64    s1 = (i64)a.x << s;
    i64    s2 = (i64)a.y << s;
    v2_i32 r  = {a.x << s, a.y << s};
    assert((i32)s1 == r.x && (i32)s2 == r.y);
    return r;
}

static inline v2_i32 v2_i32_mul(v2_i32 a, i32 s)
{
    v2_i32 r = {a.x * s, a.y * s};
    return r;
}

static inline v2_i32 v2_i32_div(v2_i32 a, i32 s)
{
    v2_i32 r = {a.x / s, a.y / s};
    return r;
}

static inline v2_i32 v2_i32_mulq(v2_i32 a, i32 n, i32 q)
{
    return v2_i32_shr(v2_i32_mul(a, n), q);
}

static inline bool32 v2_i32_eq(v2_i32 a, v2_i32 b)
{
    return a.x == b.x && a.y == b.y;
}

static inline i32 v2_i32_dot(v2_i32 a, v2_i32 b)
{
    return i32_mul(a.x, b.x) + i32_mul(a.y, b.y);
}

static inline i32 v2_i32_crs(v2_i32 a, v2_i32 b)
{
    return i32_mul(a.x, b.y) - i32_mul(a.y, b.x);
}

static inline i64 v2_i32_crsl(v2_i32 a, v2_i32 b)
{
    return (i64)a.x * b.y - (i64)a.y * b.x;
}

static inline i64 v2_i32_lensql(v2_i32 a)
{
    i64 x = (i64)a.x;
    i64 y = (i64)a.y;
    return (x * x + y * y);
}

static inline u32 v2_i32_lensq(v2_i32 a)
{
    return ((u32)pow2_i32(a.x) + (u32)pow2_i32(a.y));
}

static inline i32 v2_i32_len(v2_i32 v)
{
    return sqrt_u32(v2_i32_lensq(v));
}

static inline i32 v2_i32_len_appr(v2_i32 v)
{
    return amax_bmin_i32(v.x, v.y);
}

static inline u32 v2_i32_lenl(v2_i32 v)
{
    return sqrt_u64(v2_i32_lensql(v));
}

static inline f32 v2_i32_lenf(v2_i32 v)
{
    f32 ls = (f32)v.x * (f32)v.x + (f32)v.y * (f32)v.y;
    return sqrt_f32(ls);
}

static inline u32 v2_i32_distancesq(v2_i32 a, v2_i32 b)
{
    return v2_i32_lensq(v2_i32_sub(a, b));
}

static inline i64 v2_i32_distancesql(v2_i32 a, v2_i32 b)
{
    return v2_i32_lensql(v2_i32_sub(a, b));
}

static inline i32 v2_i32_distance_appr(v2_i32 a, v2_i32 b)
{
    return v2_i32_len_appr(v2_i32_sub(a, b));
}

static inline i32 v2_i32_distance(v2_i32 a, v2_i32 b)
{
    return sqrt_u32(v2_i32_distancesq(a, b));
}

static inline i32 v2_i32_distancel(v2_i32 a, v2_i32 b)
{
    return sqrt_u64(v2_i32_distancesql(a, b));
}

static inline i32 v2_i32_distance_sh_red(v2_i32 a, v2_i32 b, i32 sh)
{
    v2_i32 x = v2_i32_shr(a, sh);
    v2_i32 y = v2_i32_shr(b, sh);
    return ((v2_i32_distance(x, y) << sh));
}

static inline v2_i32 v2_i32_setlenl(v2_i32 a, u32 l, u32 new_l)
{
    v2_i32 r = {0};
    if (l == 0) {
        r.x = new_l;
    } else {
        r.x = (i32)(((i64)a.x * (i64)new_l) / (i64)l);
        r.y = (i32)(((i64)a.y * (i64)new_l) / (i64)l);
    }
    return r;
}

static inline v2_i32 v2_i32_setlenl_small(v2_i32 a, u32 len_curr, u32 len)
{
    v2_i32 r = {len, 0};
    if (len_curr) {
        r.x = i32_mul(a.x, (i32)len) / (i32)len_curr;
        r.y = i32_mul(a.y, (i32)len) / (i32)len_curr;
    }
    return r;
}

static inline v2_i32 v2_i32_setlen(v2_i32 a, i32 len)
{
    return v2_i32_setlenl(a, v2_i32_len(a), len);
}

static inline v2_i32 v2_i32_setlen_fast(v2_i32 a, i32 len)
{
    return v2_i32_setlenl_small(a, v2_i32_len_appr(a), len);
}

static inline v2_i32 v2_i32_truncate(v2_i32 a, u32 l)
{
    u32 ls = v2_i32_lensq(a);
    if (ls <= l * l) return a;
    return v2_i32_setlenl_small(a, sqrt_u32(ls), l);
}

static inline v2_i32 v2_i32_truncate_fast(v2_i32 a, i32 l)
{
    i32 len = v2_i32_len_appr(a);
    if (len <= l) return a;
    return v2_i32_setlenl_small(a, len, l);
}

static inline v2_i32 v2_i32_truncatel(v2_i32 a, u32 l)
{
    u64 ls = v2_i32_lensql(a);
    if (ls <= (u64)l * (u64)l) return a;
    return v2_i32_setlenl(a, sqrt_u64(ls), l);
}

static v2_i32 v2_i32_lerp(v2_i32 a, v2_i32 b, i32 num, i32 den)
{
    v2_i32 v = {lerp_i32(a.x, b.x, num, den),
                lerp_i32(a.y, b.y, num, den)};
    return v;
}

static v2_i32 v2_i32_ease(v2_i32 a, v2_i32 b, i32 num, i32 den, ease_i32 f)
{
    ease_i32 ef = f ? f : lerp_i32;
    v2_i32   v  = {ef(a.x, b.x, num, den),
                   ef(a.y, b.y, num, den)};
    return v;
}

static v2_i32 v2_i32_lerpl(v2_i32 a, v2_i32 b, i64 num, i64 den)
{
    v2_i32 v = {(i32)lerp_i64(a.x, b.x, num, den),
                (i32)lerp_i64(a.y, b.y, num, den)};
    return v;
}

// ============================================================================
// V2 I16
// ============================================================================

static inline i32 v2_i16_dot(v2_i16 a, v2_i16 b)
{
    return i16x2_dot(i16x2_ld(&a), i16x2_ld(&b));
}

static inline i32 v2_i16_crs(v2_i16 a, v2_i16 b)
{
    return i16x2_crs(i16x2_ld(&a), i16x2_ld(&b));
}

static inline i32 v2_i16_lensq(v2_i16 a)
{
    return v2_i16_dot(a, a);
}

static inline v2_i16 v2_i16_add(v2_i16 a, v2_i16 b)
{
    v2_i16 r = {0};
    i16x2_st(i16x2_add(i16x2_ld(&a), i16x2_ld(&b)), &r);
    return r;
}

static inline v2_i16 v2_i16_sub(v2_i16 a, v2_i16 b)
{
    v2_i16 r = {0};
    i16x2_st(i16x2_sub(i16x2_ld(&a), i16x2_ld(&b)), &r);
    return r;
}

static inline bool32 v2_i16_eq(v2_i16 a, v2_i16 b)
{
    return (a.x == b.x && a.y == b.y);
}

static inline v2_i16 v2_i16_shr(v2_i16 v, i32 s)
{
    v2_i16 res = {v.x >> s, v.y >> s};
    return res;
}

static inline v2_i16 v2_i16_shl(v2_i16 v, i32 s)
{
    v2_i16 res = {v.x << s, v.y << s};
    return res;
}

static inline i32 v2_i16_distancesq(v2_i16 a, v2_i16 b)
{
    return v2_i16_lensq(v2_i16_sub(a, b));
}

// ============================================================================
// V2 F32
// ============================================================================

static inline v2_f32 v2f_inv(v2_f32 a)
{
    v2_f32 r = {-a.x, -a.y};
    return r;
}

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
        return CINIT(v2_f32){len, 0.f};
    }

    v2_f32 r = {a.x * (len / l), a.y * (len / l)};
    return r;
}

static inline v2_f32 v2f_setlenl(v2_f32 a, f32 l, f32 new_l)
{
    if (l == 0.f) {
        return CINIT(v2_f32){new_l, 0.f};
    }

    v2_f32 r = {a.x * (new_l / l), a.y * (new_l / l)};
    return r;
}

static inline v2_f32 v2f_truncate(v2_f32 a, f32 len)
{
    f32 l = v2f_len(a);
    if (l < len) {
        return a;
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

// checks if r_inside is completely inside r_outside
static bool32 overlap_rec_completely(rec_i32 r_outside, rec_i32 r_inside)
{
    rec_i32 ri;
    return (intersect_rec(r_outside, r_inside, &ri) &&
            ri.w == r_inside.w && ri.h == r_inside.h);
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

    return (v2_i32_distancesq(ctr, tp) < r * r);
}

static rec_i32 translate_rec(rec_i32 r, i32 x, i32 y)
{
    rec_i32 rr = {r.x + x, r.y + y, r.w, r.h};
    return rr;
}

static tri_i32 translate_tri(tri_i32 t, v2_i32 p)
{
    tri_i32 tt = {{v2_i32_add(t.p[0], p), v2_i32_add(t.p[1], p), v2_i32_add(t.p[2], p)}};
    return tt;
}

static tri_i16 translate_tri_i16(tri_i16 t, v2_i16 p)
{
    tri_i16 tt = {{v2_i16_add(t.p[0], p),
                   v2_i16_add(t.p[1], p),
                   v2_i16_add(t.p[2], p)}};
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

// 0-----1
// |     |
// 3-----2
static void points_from_rec_i16(rec_i32 r, v2_i16 *pts)
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

static inline void intersect_line_uv_p(v2_i32 a, v2_i32 b, v2_i32 c, v2_i32 d,
                                       i32 *u, i32 *v, i32 *den, v2_i32 *p)
{
    assert(u && v && den);
    v2_i32 x = v2_i32_sub(a, c);
    v2_i32 y = v2_i32_sub(c, d);
    v2_i32 z = v2_i32_sub(a, b);
    i32    s = v2_i32_crs(x, y);
    i32    t = v2_i32_crs(x, z);
    i32    m = v2_i32_crs(z, y);
    if (u && v && den) {
        *u   = s;
        *v   = t;
        *den = m;
    }
    if (p && m) {
        p->x = a.x + ((b.x - a.x) * s) / m;
        p->y = a.y + ((b.y - a.y) * s) / m;
    }
}

// returns interpolation data for the intersection:
// S = A + [(B - A) * u] / den;
// S = C + [(D - C) * v] / den;
static inline void intersect_line_uv(v2_i32 a, v2_i32 b, v2_i32 c, v2_i32 d,
                                     i32 *u, i32 *v, i32 *den)
{
    intersect_line_uv_p(a, b, c, d, u, v, den, 0);
}

static inline void intersect_line(v2_i32 a, v2_i32 b, v2_i32 c, v2_i32 d,
                                  v2_i32 *p)
{
    intersect_line_uv_p(a, b, c, d, 0, 0, 0, p);
}

// returns interpolation data for the intersection:
// S = A + [(B - A) * u] / den;
static inline void intersect_line_u(v2_i32 a, v2_i32 b, v2_i32 c, v2_i32 d,
                                    i32 *u, i32 *den)
{
    assert(u && den);
    v2_i32 x = v2_i32_sub(a, c);
    v2_i32 y = v2_i32_sub(c, d);
    v2_i32 z = v2_i32_sub(a, b);
    *u       = v2_i32_crs(x, y);
    *den     = v2_i32_crs(z, y);
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
    v2_i32 d1 = v2_i32_sub(p, l.a);
    v2_i32 d2 = v2_i32_sub(l.b, l.a);
    return (v2_i32_crs(d1, d2) == 0 &&
            (between_excl_i32(d1.x, 0, d2.x) || //  <-- why here OR...?
             between_excl_i32(d1.y, 0, d2.y)));
}

// check if point on linesegment - on endpoints considered overlapped
static bool32 overlap_lineseg_pnt_incl(lineseg_i32 l, v2_i32 p)
{
    v2_i32 d1 = v2_i32_sub(p, l.a);
    v2_i32 d2 = v2_i32_sub(l.b, l.a);
    return (v2_i32_crs(d1, d2) == 0 &&
            (between_incl_i32(d1.x, 0, d2.x) && //  <-- ...and here AND?
             between_incl_i32(d1.y, 0, d2.y)));
}

// check if point on linesegment - on endpoints considered overlapped
static bool32 overlap_lineseg_pnt_incl_excl(lineseg_i32 l, v2_i32 p)
{
    v2_i32 d1 = v2_i32_sub(p, l.a);
    v2_i32 d2 = v2_i32_sub(l.b, l.a);
    return (v2_i32_crs(d1, d2) == 0 &&
            (between_incl_excl_i32(d1.x, 0, d2.x) || //  <-- ...and here AND?
             between_incl_excl_i32(d1.y, 0, d2.y)));
}

// check if point on linesegment - on endpoints considered NOT overlapped
static inline bool32 overlap_line_pnt(line_i32 l, v2_i32 p)
{
    return (v2_i32_crs(v2_i32_sub(l.b, l.a), v2_i32_sub(p, l.a)) == 0);
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
static ratio_s project_pnt_dir_ratio(v2_i32 pnt, v2_i32 dir)
{
    u32 d = v2_i32_lensq(dir);
    i32 s = v2_i32_dot(dir, pnt);
    assert(d <= (u32)I32_MAX);
    ratio_s r = {(i32)s, (i32)d};
    return r;
}

// projects a point onto a line and clamps it to the endpoints if not infinite
// S = A + [(B - A) * u] / den
static ratio_s project_pnt_line_ratio(v2_i32 p, v2_i32 a, v2_i32 b)
{
    v2_i32 pnt = v2_i32_sub(p, a);
    v2_i32 dir = v2_i32_sub(b, a);
    return project_pnt_dir_ratio(pnt, dir);
}

static v2_i32 project_pnt_line(v2_i32 p, v2_i32 a, v2_i32 b)
{
    ratio_s r = project_pnt_line_ratio(p, a, b);
    v2_i32  t = v2_i32_lerpl(a, b, r.num, r.den);
    return t;
}

static v2_i32 project_pnt_dir(v2_i32 p, v2_i32 dir)
{
    ratio_s r = project_pnt_dir_ratio(p, dir);
    i64     x = (((i64)dir.x * (i64)r.num) / r.den);
    i64     y = (((i64)dir.y * (i64)r.num) / r.den);
    assert(I32_MIN <= x && x <= I32_MAX);
    assert(I32_MIN <= y && y <= I32_MAX);
    v2_i32 t = {(i32)x,
                (i32)y};
    return t;
}

static i32 project_pnt_dir_len(v2_i32 p, v2_i32 dir)
{
    return v2_i32_len(project_pnt_dir(p, dir));
}

static i32 project_pnt_dir_len_appr(v2_i32 p, v2_i32 dir)
{
    return v2_i32_len_appr(project_pnt_dir(p, dir));
}

static void barycentric_uvw(v2_i32 a, v2_i32 b, v2_i32 c, v2_i32 p,
                            i32 *u, i32 *v, i32 *w)
{
    *u = v2_i32_crs(v2_i32_sub(b, a), v2_i32_sub(p, a));
    *v = v2_i32_crs(v2_i32_sub(a, c), v2_i32_sub(p, c));
    *w = v2_i32_crs(v2_i32_sub(c, b), v2_i32_sub(p, b));
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
    bool32 r = (u > 0 && v > 0 && w > 0) || (u < 0 && v < 0 && w < 0);
    return r;
}

// check for overlap - touching tri considered overlapped
static inline bool32 overlap_tri_pnt_incl2(tri_i32 t, v2_i32 p)
{
    i32 u, v, w;
    tri_pnt_barycentric_uvw(t, p, &u, &v, &w);
    return (u | v | w) >= 0 || (u <= 0 && v <= 0 && w <= 0);
}

// check for overlap - touching tri considered overlapped
static bool32 overlap_tri_pnt_incl(tri_i32 t, v2_i32 p)
{
    i32 u = v2_i32_crs(v2_i32_sub(t.p[1], t.p[0]), v2_i32_sub(p, t.p[0]));
    i32 v = v2_i32_crs(v2_i32_sub(t.p[0], t.p[2]), v2_i32_sub(p, t.p[2]));
    i32 w = v2_i32_crs(v2_i32_sub(t.p[2], t.p[1]), v2_i32_sub(p, t.p[1]));
    return (u | v | w) >= 0 || (u <= 0 && v <= 0 && w <= 0);
}

static bool32 overlap_tri_pnt_incl_i16(tri_i16 t, v2_i16 p)
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
    v2_i32 w  = v2_i32_sub(l.b, l.a);
    v2_i32 x  = v2_i32_sub(t.p[1], t.p[0]);
    v2_i32 y  = v2_i32_sub(t.p[2], t.p[0]);
    v2_i32 z  = v2_i32_sub(t.p[2], t.p[1]);
    v2_i32 a  = v2_i32_sub(l.a, t.p[0]);
    v2_i32 b  = v2_i32_sub(l.b, t.p[0]);
    v2_i32 c  = v2_i32_sub(l.a, t.p[1]);
    v2_i32 d  = v2_i32_sub(l.a, t.p[2]);
    v2_i32 e  = v2_i32_sub(l.b, t.p[1]);
    i32    a0 = v2_i32_crs(x, y);
    i32    a1 = v2_i32_crs(a, x);
    i32    a2 = v2_i32_crs(b, x);
    i32    b1 = v2_i32_crs(a, y);
    i32    b2 = v2_i32_crs(b, y);
    i32    c0 = v2_i32_crs(x, z);
    i32    c1 = v2_i32_crs(c, z);
    i32    c2 = v2_i32_crs(e, z);
    i32    d0 = v2_i32_crs(w, a);
    i32    d1 = v2_i32_crs(w, c);
    i32    d2 = v2_i32_crs(w, d);
    i32    b0 = -a0;

    bool32 separated =
        (a0 | a1 | a2) >= 0 || (a0 <= 0 && a1 <= 0 && a2 <= 0) ||
        (b0 | b1 | b2) >= 0 || (b0 <= 0 && b1 <= 0 && b2 <= 0) ||
        (c0 | c1 | c2) >= 0 || (c0 <= 0 && c1 <= 0 && c2 <= 0) ||
        (d0 | d1 | d2) >= 0 || (d0 <= 0 && d1 <= 0 && d2 <= 0);
    return !separated;
}

static bool32 overlap_tri_lineseg_excl_i16(tri_i16 t, lineseg_i16 l)
{
    v2_i16 w  = v2_i16_sub(l.b, l.a);
    v2_i16 x  = v2_i16_sub(t.p[1], t.p[0]);
    v2_i16 y  = v2_i16_sub(t.p[2], t.p[0]);
    v2_i16 z  = v2_i16_sub(t.p[2], t.p[1]);
    v2_i16 a  = v2_i16_sub(l.a, t.p[0]);
    v2_i16 b  = v2_i16_sub(l.b, t.p[0]);
    v2_i16 c  = v2_i16_sub(l.a, t.p[1]);
    v2_i16 d  = v2_i16_sub(l.a, t.p[2]);
    v2_i16 e  = v2_i16_sub(l.b, t.p[1]);
    i32    a0 = v2_i16_crs(x, y);
    i32    a1 = v2_i16_crs(a, x);
    i32    a2 = v2_i16_crs(b, x);
    i32    b1 = v2_i16_crs(a, y);
    i32    b2 = v2_i16_crs(b, y);
    i32    c0 = v2_i16_crs(x, z);
    i32    c1 = v2_i16_crs(c, z);
    i32    c2 = v2_i16_crs(e, z);
    i32    d0 = v2_i16_crs(w, a);
    i32    d1 = v2_i16_crs(w, c);
    i32    d2 = v2_i16_crs(w, d);
    i32    b0 = -a0;

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

    v2_i32 dt        = v2_i32_sub(l.b, l.a);
    i32    a0        = v2_i32_crs(v2_i32_sub(p[0], l.a), dt);
    i32    a1        = v2_i32_crs(v2_i32_sub(p[1], l.a), dt);
    i32    a2        = v2_i32_crs(v2_i32_sub(p[2], l.a), dt);
    i32    a3        = v2_i32_crs(v2_i32_sub(p[3], l.a), dt);
    bool32 separated = (a0 | a1 | a2 | a3) >= 0 ||
                       (a0 <= 0 && a1 <= 0 && a2 <= 0 && a3 <= 0);
    return !separated;
}

// paulbourke.net/geometry/circlesphere/
static i32 intersect_cir(v2_i32 a, i32 ra, v2_i32 b, i32 rb,
                         v2_i32 *u, v2_i32 *v)
{
    v2_i32 ab = v2_i32_sub(b, a);
    i32    ls = v2_i32_lensq(ab);
    if (pow2_i32(ra + rb) < ls || ls < pow2_i32(ra - rb)) return 0;
    if (ls == 0) return (ra == rb ? -1 : 0);
    i32 ls2 = ls << 1;
    i32 ls4 = ls << 2;
    i32 k   = ra * ra - rb * rb + ls;
    i32 h   = sqrt_i32(ra * ra - (k * k + ls2) / ls4);
    i32 l   = sqrt_i32(ls);
    i32 px  = a.x + (k * ab.x + ls) / ls2;
    i32 py  = a.y + (k * ab.y + ls) / ls2;
    i32 dx  = (h * ab.y) / l;
    i32 dy  = (h * ab.x) / l;
    if (u) {
        u->x = px + dx, u->y = py - dy;
    }
    if (v) {
        v->x = px - dx, v->y = py + dy;
    }
    return (1 + (dx != 0 || dy != 0));
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
    m33_f32 m = {0};
    for (i32 n = 0; n < 9; n++)
        m.m[n] = a.m[n] + b.m[n];
    return m;
}

static m33_f32 m33_sub(m33_f32 a, m33_f32 b)
{
    m33_f32 m = {0};
    for (i32 n = 0; n < 9; n++)
        m.m[n] = a.m[n] - b.m[n];
    return m;
}

static m33_f32 m33_mul(m33_f32 a, m33_f32 b)
{
    m33_f32 m = {0};
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
    v2_f32 r = {a.x + (b.x - a.x) * t,
                a.y + (b.y - a.y) * t};
    return r;
}

// return a point along a spline with n_p points
static v2_i32 v2_i32_spline_internal(v2_i32 *p, i32 n_p, i32 n, u16 k_q16, b32 circ, b32 halved)
{
    i32 p0, p1, p2, p3;

    if (circ) {
        p1 = n;
        p2 = (p1 + 1) % n_p;
        p3 = (p1 + 2) % n_p;
        p0 = (p1 - 1 + n_p) % n_p;
    } else {
        p1 = n + 1;
        p2 = p1 + 1;
        p3 = p1 + 2;
        p0 = p1 - 1;
    }

    i32 k   = (i32)k_q16;
    i32 kk  = (i32)(((i64)k * k + 32768) >> 16);
    i32 kkk = (i32)(((i64)kk * k + 32768) >> 16);
    i32 q1  = -1 * kkk + 2 * kk - k;
    i32 q2  = +3 * kkk - 5 * kk + 2 * 65536;
    i32 q3  = -3 * kkk + 4 * kk + k;
    i32 q4  = +1 * kkk - 1 * kk;

    i64 tx = ((i64)p[p0].x * q1) +
             ((i64)p[p1].x * q2) +
             ((i64)p[p2].x * q3) +
             ((i64)p[p3].x * q4);
    i64 ty = ((i64)p[p0].y * q1) +
             ((i64)p[p1].y * q2) +
             ((i64)p[p2].y * q3) +
             ((i64)p[p3].y * q4);

    v2_i32 r = {(i32)(tx >> 16), (i32)(ty >> 16)};
    if (halved) {
        r.x >>= 1;
        r.y >>= 1;
    }
    return r;
}

// return a point along a spline with n_p points
static v2_i32 v2_i32_spline(v2_i32 *p, i32 n_p, i32 n, u16 k_q16, b32 circ)
{
    return v2_i32_spline_internal(p, n_p, n, k_q16, circ, 1);
}

static i32 v2_i32_spline_seg_len(v2_i32 *p, i32 n_p, i32 node_from, b32 circ)
{
    i32    l     = 0;
    v2_i32 p_old = v2_i32_spline_internal(p, n_p, node_from, 0, circ, 0);

    for (i32 k_q16 = 0; k_q16 < 65536; k_q16 += 4096) {
        v2_i32 p_new = v2_i32_spline_internal(p, n_p, node_from, k_q16, circ, 0);
        l += v2_i32_distance(p_old, p_new);
        p_old = p_new;
    }
    return (l >> 1);
}

static i32 v2_i32_spline_len(v2_i32 *p, i32 n_p, b32 circ)
{
    i32 l = 0;
    for (i32 n = 0; n < n_p; n++) {
        l += v2_i32_spline_seg_len(p, n_p, n, circ);
    }
    return l;
}

// 0x1021 polynomial
static u16 crc16_next(u16 c, u8 v)
{
    u16 r = (u16)v;
    r ^= (c >> 8);
    r ^= (r >> 4);
    r ^= (r << 5) ^ (r << 12);
    r ^= (c << 8);
    return r;
}

static u16 crc16(const void *p, usize size)
{
    // 0xFFFF initial value
    // 0x1021 polynomial
    const u8 *d = (const u8 *)p;
    u16       a = 0xFFFF;

    for (usize n = 0; n < size; n++) {
        a = crc16_next(a, (u16)(*d++));
    }
    return a;
}

#endif