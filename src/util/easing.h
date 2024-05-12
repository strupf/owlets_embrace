// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef EASING_H
#define EASING_H

#include "mathfunc.h"

// https://easings.net/

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
#endif