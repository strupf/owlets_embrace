// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef EASING_H
#define EASING_H

#include "mathfunc.h"

// https://easings.net/

static inline i32 ease_lin(i32 a, i32 b, i32 num, i32 den)
{
    i32 i = (b - a) * num;
#ifdef PLTF_SDL
    i64 j = (i64)(b - a) * (i64)num;
    assert((i64)i == j);
#endif
    return (a + i / den);
}

static i32 ease_in_sine(i32 a, i32 b, i32 num, i32 den)
{
    i32 x  = (i32)(((i64)num << 16) / den);
    i32 ab = b - a;
    return a + ab - ((ab * cos_q16(x)) >> 16);
}

static i32 ease_out_sine(i32 a, i32 b, i32 num, i32 den)
{
    i32 x = (i32)(((i64)num << 16) / den);
    return a + (((b - a) * sin_q16(x)) >> 16);
}

static i32 ease_in_out_sine(i32 a, i32 b, i32 num, i32 den)
{
    i32 x  = (i32)(((i64)num << 17) / den);
    i32 ab = b - a;
    return a - (((ab * cos_q16(x)) >> 16) - ab) / 2;
}

static i32 ease_in_quad(i32 a, i32 b, i32 num, i32 den)
{
    return a + (i32)(((i64)(b - a) * num * num) / (i64)(den * den));
}

static i32 ease_out_quad(i32 a, i32 b, i32 num, i32 den)
{
    i32 ab = b - a;
    i32 n  = den - num;
    return a + ab - (i32)(((i64)ab * n * n) / ((i64)den * den));
}

static i32 ease_in_out_quad(i32 a, i32 b, i32 num, i32 den)
{
    if (num < den / 2) {
        i32 n = num;
        return a + (i32)(((i64)2 * (b - a) * n * n) / (i64)(den * den));
    } else {
        i32 n = den - num;
        return b - (i32)(((i64)2 * (b - a) * n * n) / (i64)(den * den));
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

static i32 ease_out_bounce(i32 a, i32 b, i32 num, i32 den)
{
    f32 x = (f32)num / (f32)den;
    f32 r = 0.f;
    if (x < 1.f / 2.75f) {
        r = 7.5625f * x * x;
    } else if (x < 2.f / 2.75f) {
        r = 7.5625f * x * (x - 1.5f / 2.75f) + 0.75f;
    } else if (x < 2.5f / 2.75f) {
        r = 7.5625f * x * (x - 2.25f / 2.75f) + 0.9375f;
    } else {
        r = 7.5625f * x * (x - 2.625f / 2.75f) + 0.984375f;
    }
    return (a + (i32)((f32)(b - a) * r));
}

#endif