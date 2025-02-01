// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef SORTING_H
#define SORTING_H

#include "pltf/pltf_types.h"

// i32 (*cmp)(const void *a, const void *b)
// -: a goes before b | b goes after a
// 0: equivalent
// +: a goes after b | b goes before a
typedef i32 (*cmp_f)(const void *a, const void *b);

static void sort_i_array(byte *arr, byte *lo, byte *hi, usize s, cmp_f cmp);

static void sort_array(void *arr, i32 num, usize s, cmp_f cmp)
{
    if (num <= 1) return;
    assert(arr && s && cmp);
    sort_i_array((byte *)arr, (byte *)arr, (byte *)arr + (num - 1) * s, s, cmp);
}

// quicksort
static void sort_i_array(byte *arr, byte *lo, byte *hi, usize s, cmp_f c)
{
    byte m[256];

    assert(s * 2 <= sizeof(m));
    assert(lo < hi);

    byte *i = lo;
    byte *j = hi;
    byte *p = &m[s];

    mcpy(p, arr + (((lo - arr) + (hi - arr)) / (s << 1)) * s, s);

    do {
        while (c((const void *)i, (const void *)p) < 0) { i += s; }
        while (c((const void *)j, (const void *)p) > 0) { j -= s; }
        if (i > j) break;

        mcpy(m, (const void *)i, s);
        mcpy(i, (const void *)j, s);
        mcpy(j, (const void *)m, s);
        i += s;
        j -= s;
    } while (i <= j);

    if (lo < j) sort_i_array(arr, lo, j, s, c);
    if (hi > i) sort_i_array(arr, i, hi, s, c);
}

#define SORT_ARRAY_DEF(TYPE, NAME, CMPF)                      \
    static void sort_i_##NAME(TYPE *arr, TYPE *lo, TYPE *hi); \
                                                              \
    static void sort_##NAME(TYPE *arr, i32 num)               \
    {                                                         \
        if (num <= 1) return;                                 \
        assert(arr);                                          \
        sort_i_##NAME(arr, &arr[0], &arr[num - 1]);           \
    }                                                         \
                                                              \
    static void sort_i_##NAME(TYPE *arr, TYPE *lo, TYPE *hi)  \
    {                                                         \
        assert(lo < hi);                                      \
        TYPE  p = arr[((lo - arr) + (hi - arr)) >> 1];        \
        TYPE *i = lo;                                         \
        TYPE *j = hi;                                         \
                                                              \
        do {                                                  \
            while (CMPF(i, &p) < 0) { i++; }                  \
            while (CMPF(j, &p) > 0) { j--; }                  \
            if (i > j) break;                                 \
            TYPE tp = *i;                                     \
            *i      = *j;                                     \
            *j      = tp;                                     \
            i++, j--;                                         \
        } while (i <= j);                                     \
                                                              \
        if (lo < j) sort_i_##NAME(arr, lo, j);                \
        if (hi > i) sort_i_##NAME(arr, i, hi);                \
    }

#endif
