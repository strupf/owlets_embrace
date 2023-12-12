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
    int x  = (num * (Q16_ANGLE_TURN >> 2)) / den;
    int ab = b - a;
    return a + ((ab * sin_q16(x)) >> 16);
}

static int ease_in_out_sine(int a, int b, int num, int den)
{
    int x  = (num * (Q16_ANGLE_TURN >> 1)) / den;
    int ab = b - a;
    return a - (((ab * cos_q16(x)) >> 16) - ab) / 2;
}

static int ease_in_quad(int a, int b, int num, int den)
{
    int ab = b - a;
    return a + (ab * num * num) / (den * den);
}

static int ease_out_quad(int a, int b, int num, int den)
{
    int ab = b - a;
    int n  = den - num;
    return a + ab - (ab * n * n) / (den * den);
}

static int ease_in_out_quad(int a, int b, int num, int den)
{
    int ab = b - a;

    if (num < den / 2) {
        return a + (2 * ab * num * num) / (den * den);
    } else {
        int n = den - num;
        return b - (2 * ab * n * n) / (den * den);
    }
}
#endif