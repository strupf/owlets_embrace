// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef SORTING_H
#define SORTING_H

// insertion sort
//
// Compared to qsort this sort method is stable i.e. objects
// which have an equal ranking stay in the same order as before.
// Qsort might swap them seemingly at random which causes them to "fight"
// which looks terrible during rendering

#include "pltf/pltf_types.h"

// i32 (*cmp)(const void *a, const void *b)
// -: a goes before b | b goes after a
// 0: equivalent
// +: a goes after b | b goes before a
typedef i32 (*cmp_f)(const void *a, const void *b);

// insertion sort
static void sort_array(void *arr, i32 n, usize s, cmp_f cmp)
{
    ALIGNAS(32) byte m[1024];

    assert(arr && s && cmp);
    assert(s <= sizeof(m));

    byte *ae = (byte *)arr + s * n;
    for (byte *ei = (byte *)arr + s; ei < ae; ei += s) {
        mcpy(m, ei, s);
        byte *ej = ei;

        while ((byte *)arr < ej && 0 < cmp((const void *)(ej - s), (const void *)m)) {
            mcpy(ej, ej - s, s);
            ej -= s;
        }
        if (ej != ei) {
            mcpy(ej, m, s);
        }
    }
}

#define SORT_ARRAY_DEF(T, NAME, CMPF)                  \
    static void sort_##NAME(T *arr, i32 n)             \
    {                                                  \
        T *ae = arr + n;                               \
        for (T *ei = arr + 1; ei < ae; ei++) {         \
            T  t  = *ei;                               \
            T *ej = ei;                                \
                                                       \
            while (arr < ej && 0 < CMPF(ej - 1, &t)) { \
                *ej = *(ej - 1);                       \
                ej--;                                  \
            }                                          \
            *ej = t;                                   \
        }                                              \
    }

#endif
