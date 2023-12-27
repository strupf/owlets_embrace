// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef EASING_H
#define EASING_H

#include "mathfunc.h"

// https://easings.net/

static int ease_in_sine(int a, int b, int num, int den)
{
    int x  = (num * (Q16_ANGLE_TURN >> 2)) / den;
    int ab = b - a;
    return a + ab - ((ab * cos_q16(x)) >> 16);
}

static int ease_out_sine(int a, int b, int num, int den)
{
    int x = (num * (Q16_ANGLE_TURN >> 2)) / den;
    return a + (((b - a) * sin_q16(x)) >> 16);
}

static int ease_in_out_sine(int a, int b, int num, int den)
{
    int x  = (num * (Q16_ANGLE_TURN >> 1)) / den;
    int ab = b - a;
    return a - (((ab * cos_q16(x)) >> 16) - ab) / 2;
}

static int ease_in_quad(int a, int b, int num, int den)
{
    return a + ((b - a) * num * num) / (den * den);
}

static int ease_out_quad(int a, int b, int num, int den)
{
    int ab = b - a;
    int n  = den - num;
    return a + ab - (ab * n * n) / (den * den);
}

static int ease_in_out_quad(int a, int b, int num, int den)
{
    if (num < den / 2) {
        int n = num;
        return a + (2 * (b - a) * n * n) / (den * den);
    } else {
        int n = den - num;
        return b - (2 * (b - a) * n * n) / (den * den);
    }
}
#endif