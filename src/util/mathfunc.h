// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef MATHFUNC_H
#define MATHFUNC_H

#include "sys/sys_intrin.h"
#include "sys/sys_types.h"

#define PI_FLOAT  3.1415927f
#define PI2_FLOAT 6.2831853f

static inline int clamp_i(int x, int lo, int hi)
{
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

static inline int min_i(int a, int b)
{
    return (a < b ? a : b);
}

static inline int max_i(int a, int b)
{
    return (a > b ? a : b);
}

static inline int abs_i(int a)
{
    return (a < 0 ? -a : a);
}

static inline int sgn_i(int a)
{
    if (a < 0) return -1;
    if (a > 0) return +1;
    return 0;
}

static inline i32 clamp_i32(i32 x, i32 lo, i32 hi)
{
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

static inline f32 clamp_f(f32 x, f32 lo, f32 hi)
{
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

static inline i32 min_i32(i32 a, i32 b)
{
    return (a < b ? a : b);
}

static inline i32 max_i32(i32 a, i32 b)
{
    return (a > b ? a : b);
}

static inline i32 abs_i32(i32 a)
{
    return (a < 0 ? -a : a);
}

static inline i32 sgn_i32(i32 a)
{
    if (a < 0) return -1;
    if (a > 0) return +1;
    return 0;
}

static inline i32 pow_i32(i32 v, i32 power)
{
    i32 r = v;
    for (int n = 1; n < power; n++) {
        r *= v;
    }
    return r;
}

static inline i32 pow2_i32(i32 v)
{
    return (v * v);
}

static inline int log2_u32(u32 v)
{
    return (31 - (int)clz32(v));
}

static inline bool32 is_pow2_u32(u32 v)
{
    return ((v & (v - 1)) == 0);
}

// rounded division - stackoverflow.com/a/18067292
static inline i32 divr_i32(i32 n, i32 d)
{
    i32 h = d / 2;
    return ((n < 0 && d < 0) ? n + h : n - h) / d;
}

static inline i32 lerp_i32(i32 a, i32 b, i32 num, i32 den)
{
    return (a + ((b - a) * num) / den);
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

static inline u32 sqrt_u32(u32 x)
{
    return (u32)sqrtf((f32)x);
}

static inline i32 sqrt_i32(i32 x)
{
    return (i32)sqrtf((f32)x);
}

#define Q16_ANGLE_TURN 0x40000

// p: angle, where 2 PI = 262144 = 0x40000
// output: [-65536, 65536] = [-1; +1]
static i32 cos_q16(i32 p)
{
    u32 i   = (u32)p & 0x3FFFF; // (p >= 0 ? (u32)p : (u32)(-p)) & 0x3FFFF;
    int neg = (0x10000 <= i && i < 0x30000);
    switch (i >> 16) {
    case 1: i = 0x20000 - i; break; // [65536, 131071]
    case 2: i = i - 0x20000; break; // [131072, 196607]
    case 3: i = 0x40000 - i; break; // [196608, 262143]
    }
    if (i == 0x10000) return 0;
    i = (i * i + 0x8000) >> 16;
    u32 r;
    r = 0x00002;                   // Constants multiplied by scaling:
    r = 0x0003C - ((i * r) >> 16); // (PI/2)^10 / 3628800
    r = 0x00557 - ((i * r) >> 16); // (PI/2)^8 / 40320
    r = 0x040F0 - ((i * r) >> 16); // (PI/2)^6 / 720
    r = 0x13BD4 - ((i * r) >> 16); // (PI/2)^4 / 24
    r = 0x10000 - ((i * r) >> 16); // (PI/2)^2 / 2
    return neg ? -(i32)r : (i32)r;
}

// p: angle, where 2 PI = 262144 = 0x40000
// output: [-65536, 65536] = [-1; +1]
static i32 cos_q16_fast(i32 p)
{
    u32 i   = (u32)p & 0x3FFFF; // (p >= 0 ? (u32)p : (u32)(-p)) & 0x3FFFF;
    int neg = (0x10000 <= i && i < 0x30000);
    switch (i >> 16) {
    case 1: i = 0x20000 - i; break; // [65536, 131071]
    case 2: i = i - 0x20000; break; // [131072, 196607]
    case 3: i = 0x40000 - i; break; // [196608, 262143]
    }
    if (i == 0x10000) return 0;
    i = (i * i) >> 16;
    u32 r;                         // 2 less terms than the other cos
    r = 0x0051F;                   // Constants multiplied by scaling:
    r = 0x040F0 - ((i * r) >> 16); // (PI/2)^6 / 720
    r = 0x13BD3 - ((i * r) >> 16); // (PI/2)^4 / 24
    r = 0x10000 - ((i * r) >> 16); // (PI/2)^2 / 2
    return neg ? -(i32)r : (i32)r;
}

// p: angle, where 262144 = 0x40000 = 2 PI
// output: [-65536, 65536] = [-1; +1]
static inline i32 sin_q16(i32 p)
{
    return cos_q16(p - 0x10000);
}

// p: angle, where 262144 = 0x40000 = 2 PI
// output: [-65536, 65536] = [-1; +1]
static inline i32 sin_q16_fast(i32 p)
{
    return cos_q16_fast(p - 0x10000);
}

// input: [0,65536] = [0,1]
// output: [0, 32768] = [0, PI/4], 65536 = PI/2
static i32 atan_q16(i32 x)
{
    if (x == 0) return 0;

    // taylor expansion works for x [0, 1]
    // for bigger x: atan(x) = PI/2 - atan(1/x)
    i32 add = 0, mul = +1;
    u32 i = (u32)abs_i(x);
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
    u32 i = (u32)abs_i(x);
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

static float sin_f(float x)
{
    float i = fmodf(x, PI2_FLOAT) / PI2_FLOAT;
    return ((float)sin_q16((i32)(i * (float)Q16_ANGLE_TURN)) / 65536.f);
}

static float cos_f(float x)
{
    float i = fmodf(x, PI2_FLOAT) / PI2_FLOAT;
    return ((float)cos_q16((i32)(i * (float)Q16_ANGLE_TURN)) / 65536.f);
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

static inline v2_i32 v2_shr(v2_i32 a, int s)
{
    v2_i32 r = {a.x >> s, a.y >> s};
    return r;
}

static inline v2_i32 v2_shl(v2_i32 a, int s)
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

static inline int v2_eq(v2_i32 a, v2_i32 b)
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
    u32 x = abs_i32(a.x);
    u32 y = abs_i32(a.y);
    return x * x + y * y;
}

static inline f32 v2_lensq_f(v2_i32 v)
{
    return (f32)v.x * (f32)v.x + (f32)v.y * (f32)v.y;
}

static inline i32 v2_len(v2_i32 v)
{
    return (i32)(sqrt_f32(v2_lensq_f(v)));
}

static inline f32 v2_len_f(v2_i32 v)
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

static inline v2_i32 v2_setlen(v2_i32 a, i32 len)
{
    f32 l = v2_len_f(a);
    if (l == 0.f) {
        v2_i32 r = {len, 0};
        return r;
    }
    f32    scl = (f32)len / l;
    i32    x   = (i32)((f32)a.x * scl + 0.5f);
    i32    y   = (i32)((f32)a.y * scl + 0.5f);
    v2_i32 r   = {x, y};
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

static v2_i32 v2_lerp(v2_i32 a, v2_i32 b, i32 num, i32 den)
{
    int sig = sgn_i32(num) * sgn_i32(den);

    num = abs_i32(num);
    den = abs_i32(den);
    while (num >= 0x10000) { // reduce precision to avoid overflow
        num >>= 1;
        den >>= 1;
    }

    i32    x = sig * lerp_i32(a.x, b.x, num, den);
    i32    y = sig * lerp_i32(a.y, b.y, num, den);
    v2_i32 v = {x, y};
    return v;
}

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
    rec_i32 rec = {rx1, ry1, rx2 - rx1, ry2 - ry1};
    *r          = rec;
    return 1;
}

static inline bool32 overlap_rec_pnt_incl(rec_i32 a, v2_i32 p)
{
    return (a.x <= p.x && p.x <= a.x + a.w &&
            a.y <= p.y && p.y <= a.y + a.h);
}

static inline bool32 overlap_rec_pnt_excl(rec_i32 a, v2_i32 p)
{
    return (a.x < p.x && p.x < a.x + a.w &&
            a.y < p.y && p.y < a.y + a.h);
}

// check for overlap - touching rectangles considered overlapped
static inline bool32 overlap_rec_incl(rec_i32 a, rec_i32 b)
{
    return (!((a.x + a.w < b.x) || (a.y + a.h < b.y) ||
              (b.x + b.w < a.x) || (b.y + b.h < a.y)));
}

// check for overlap - touching rectangles considered NOT overlapped
static inline bool32 overlap_rec(rec_i32 a, rec_i32 b)
{
    return (!((a.x + a.w <= b.x) || (a.y + a.h <= b.y) ||
              (b.x + b.w <= a.x) || (b.y + b.h <= a.y)));
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
    tri_i32 tt = {v2_add(t.p[0], p), v2_add(t.p[1], p), v2_add(t.p[2], p)};
    return tt;
}

// turns an AABB into two triangles
static void tris_from_rec(rec_i32 r, tri_i32 tris[2])
{
    tri_i32 t1 = {r.x, r.y,
                  r.x + r.w, r.y,
                  r.x + r.w, r.y + r.h};
    tri_i32 t2 = {r.x, r.y,
                  r.x + r.w, r.y + r.h,
                  r.x, r.y + r.h};
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
static inline void project_pnt_line_u(v2_i32 p, v2_i32 a, v2_i32 b,
                                      i32 *u, i32 *den)
{
    v2_i32 pnt = v2_sub(p, a);
    v2_i32 dir = v2_sub(b, a);
    u32    d   = v2_lensq(dir);
    i32    s   = v2_dot(dir, pnt);
    if (d > I32_MAX) { // reduce precision to fit both into i32
        d >>= 1;
        s >>= 1;
    }
    *den = (i32)d;
    *u   = s;
}

static v2_i32 project_pnt_line(v2_i32 p, v2_i32 a, v2_i32 b)
{
    i32 u, den;
    project_pnt_line_u(p, a, b, &u, &den);
    v2_i32 t = v2_lerp(a, b, u, den);
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
    for (int n = 0; n < 3; n++) {
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

#endif