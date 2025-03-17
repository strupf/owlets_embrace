// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef EASING_H
#define EASING_H

#include "mathfunc.h"

// https://easings.net/

static inline i32 ease_lin(i32 a, i32 b, i32 num, i32 den)
{
    return lerp_i32(a, b, num, den);
}

static i32 ease_in_sine(i32 a, i32 b, i32 num, i32 den)
{
    i32 x  = (i32)(((i64)num << 15) / den);
    i32 ab = b - a;
    return a + ab - ((ab * cos_q15(x)) >> 15);
}

static i32 ease_out_sine(i32 a, i32 b, i32 num, i32 den)
{
    i32 x = (i32)(((i64)num << 15) / den);
    return a + (((b - a) * sin_q15(x)) >> 15);
}

static i32 ease_in_out_sine(i32 a, i32 b, i32 num, i32 den)
{
    i32 x  = (i32)(((i64)num << 16) / den);
    i32 ab = b - a;
    return a - (((ab * cos_q15(x)) >> 15) - ab) / 2;
}

static i32 ease_in_quad(i32 a, i32 b, i32 num, i32 den)
{
    i64 i0 = (i64)num * num;
    i64 i1 = (i64)den * den;
    i32 ab = b - a;
    return a + (i32)((i0 * ab) / i1);
}

static i32 ease_out_quad(i32 a, i32 b, i32 num, i32 den)
{
    i32 n  = den - num;
    i64 i0 = (i64)n * n;
    i64 i1 = (i64)den * den;
    i32 ab = b - a;
    return b - (i32)((i0 * ab) / i1);
}

static i32 ease_in_out_quad(i32 a, i32 b, i32 num, i32 den)
{
    i32 ab = b - a;
    if (num < den / 2) {
        i32 n = num;
        return a + (i32)(((i64)2 * ab * n * n) / (i64)(den * den));
    } else {
        i32 n = den - num;
        return b - (i32)(((i64)2 * ab * n * n) / (i64)(den * den));
    }
}

static i32 ease_out_back(i32 a, i32 b, i32 num, i32 den)
{
    f32 x = (f32)num / (f32)den;
    f32 r = 1.f + 2.70158f * pow_f32(x - 1.f, 3) + 1.70158f * pow_f32(x - 1.f, 2);
    return (a + (i32)((f32)(b - a) * r));
}

static i32 ease_out_elastic(i32 a, i32 b, i32 num, i32 den)
{
    f32 x = (f32)num / (f32)den;

    if (x <= 0.01f) return a;
    if (0.99f <= x) return b;
    f32 r = 1.f + powf(2.f, -10.f * x) * sinf((x * 10.f - 0.75f) * PI2_FLOAT / 3.f);
    return (a + (i32)((f32)(b - a) * r));
}

#endif