// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef SORTING_H
#define SORTING_H

#include "sys/sys_types.h"

#include <stdlib.h>

// int (*cmp)(const void *a, const void *b)
// -: a goes before b | b goes after a
// 0: equivalent
// +: a goes after b | b goes before a
static inline void sort_array(void *base,
                              int   num,
                              usize s,
                              int (*cmp)(const void *a, const void *b))
{
    qsort(base, (size_t)num, (size_t)s, cmp);
}

#endif