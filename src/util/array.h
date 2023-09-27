// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================
/*
 * Macros to generate array types working with a fixed sized buffer.
 *
 * ARR_DEF_BASE - array without find/contains/delete
 * ARR_DEF_PRIMITVE - array of a primitive type comparable using "=="
 * ARR_DEF - array of any type. Have to provide an equal function:
 *           static inline bool32 NAME##_equal(const T *a, const T *b);
 */

#ifndef ARRAY_H
#define ARRAY_H

#include "os/os_types.h"

/*
typedef struct {
        void **data;
        i16   *d;
        i16   *s;
        int    n;
        int    c;

        u32 *free;
        int  n_free;
} slotmap_s;

u32 slotmap_new(slotmap_s *sm)
{
        if (sm->n_free > 0) {
                u32 i = sm->free[--sm->n_free];
                return i;
        }
}

void slotmap_del(slotmap_s *sm, int i)
{
}
*/

#define ARR_DEF_BASE(NAME, T)                                                 \
        typedef struct {                                                      \
                T  *data;                                                     \
                int n;                                                        \
                int c;                                                        \
        } NAME##_arr;                                                         \
                                                                              \
        static NAME##_arr *NAME##_arrcreate(int cap, void *(*allocf)(size_t)) \
        {                                                                     \
                size_t      s   = sizeof(NAME##_arr) + sizeof(T) * cap;       \
                void       *mem = allocf(s);                                  \
                NAME##_arr *arr = (NAME##_arr *)mem;                          \
                arr->data       = (T *)(arr + 1);                             \
                arr->n          = 0;                                          \
                arr->c          = cap;                                        \
                return arr;                                                   \
        }                                                                     \
                                                                              \
        static void NAME##_arrpush(NAME##_arr *arr, T v)                      \
        {                                                                     \
                ASSERT(arr->n < arr->c);                                      \
                arr->data[arr->n++] = v;                                      \
        }                                                                     \
                                                                              \
        static void NAME##_arrinsert(NAME##_arr *arr, T v, int i)             \
        {                                                                     \
                ASSERT(arr->n < arr->c && i <= arr->n);                       \
                for (int n = arr->n; n > i; n--)                              \
                        arr->data[n] = arr->data[n - 1];                      \
                arr->data[i] = v;                                             \
                arr->n++;                                                     \
        }                                                                     \
                                                                              \
        static void NAME##_arrinsertq(NAME##_arr *arr, T v, int i)            \
        {                                                                     \
                ASSERT(arr->n < arr->c && i <= arr->n);                       \
                arr->data[arr->n++] = arr->data[i];                           \
                arr->data[i]        = v;                                      \
        }                                                                     \
                                                                              \
        static void NAME##_arrclr(NAME##_arr *arr)                            \
        {                                                                     \
                arr->n = 0;                                                   \
        }                                                                     \
                                                                              \
        static bool32 NAME##_arrdelatq(NAME##_arr *arr, int i)                \
        {                                                                     \
                if (!(0 <= i && i < arr->n)) return 0;                        \
                arr->data[i] = arr->data[--arr->n];                           \
                return 1;                                                     \
        }                                                                     \
                                                                              \
        static bool32 NAME##_arrdelat(NAME##_arr *arr, int i)                 \
        {                                                                     \
                if (!(0 <= i && i < arr->n)) return 0;                        \
                arr->n--;                                                     \
                for (int n = i; n < arr->n; n++)                              \
                        arr->data[n] = arr->data[n + 1];                      \
                return 1;                                                     \
        }                                                                     \
                                                                              \
        static T NAME##_arrat(NAME##_arr *arr, int i)                         \
        {                                                                     \
                ASSERT(0 <= i && i < arr->n);                                 \
                return arr->data[i];                                          \
        }                                                                     \
                                                                              \
        static T *NAME##_arratp(NAME##_arr *arr, int i)                       \
        {                                                                     \
                ASSERT(0 <= i && i < arr->n);                                 \
                return &arr->data[i];                                         \
        }                                                                     \
                                                                              \
        static T NAME##_arrpop(NAME##_arr *arr)                               \
        {                                                                     \
                ASSERT(arr->n > 0);                                           \
                return arr->data[--arr->n];                                   \
        }                                                                     \
                                                                              \
        static int NAME##_arrlen(NAME##_arr *arr)                             \
        {                                                                     \
                return arr->n;                                                \
        }

#define _ARR_DEF_EXTENSION(NAME, T)                            \
        static bool32 NAME##_arrcontains(NAME##_arr *arr, T v) \
        {                                                      \
                return (NAME##_arrfind(arr, v) >= 0);          \
        }                                                      \
                                                               \
        static void NAME##_arrpushunique(NAME##_arr *arr, T v) \
        {                                                      \
                if (!NAME##_arrcontains(arr, v))               \
                        NAME##_arrpush(arr, v);                \
        }                                                      \
                                                               \
        static bool32 NAME##_arrdel(NAME##_arr *arr, T v)      \
        {                                                      \
                int i = NAME##_arrfind(arr, v);                \
                if (i < 0) return 0;                           \
                NAME##_arrdelat(arr, i);                       \
                return 1;                                      \
        }                                                      \
                                                               \
        static bool32 NAME##_arrdelq(NAME##_arr *arr, T v)     \
        {                                                      \
                int i = NAME##_arrfind(arr, v);                \
                if (i < 0) return 0;                           \
                NAME##_arrdelatq(arr, i);                      \
                return 1;                                      \
        }

#define ARR_DEF_PRIMITIVE(NAME, T)                      \
        ARR_DEF_BASE(NAME, T)                           \
        static int NAME##_arrfind(NAME##_arr *arr, T v) \
        {                                               \
                for (int i = 0; i < arr->n; i++)        \
                        if (arr->data[i] == v)          \
                                return i;               \
                return -1;                              \
        }                                               \
        _ARR_DEF_EXTENSION(NAME, T)

#define ARR_DEF(NAME, T)                                           \
        ARR_DEF_BASE(NAME, T)                                      \
        static inline bool32 NAME##_equal(const T *a, const T *b); \
                                                                   \
        static int NAME##_arrfind(NAME##_arr *arr, T v)            \
        {                                                          \
                for (int i = 0; i < arr->n; i++)                   \
                        if (NAME##_equal(&arr->data[i], &v))       \
                                return i;                          \
                return -1;                                         \
        }                                                          \
        _ARR_DEF_EXTENSION(NAME, T)

typedef struct {
        v2_i32 *data;
        int     n;
        int     c;
} v2_arr;

typedef struct {
        int    i;
        v2_i32 e;
} v2_arr_it;

static bool32 v2_arr_next(v2_arr *arr, v2_arr_it *it)
{
        if (it->i >= arr->n) return 0;
        it->e = arr->data[it->i];
        return 1;
}

#define v2_arr_foreach(ARRPTR, ITNAME) \
        for (v2_arr_it ITNAME = {0}; v2_arr_next(ARRPTR, &ITNAME); ITNAME.i++)
#define foreach_v2_arr(ARRPTR, ITNAME) \
        for (v2_arr_it ITNAME = {0}; v2_arr_next(ARRPTR, &ITNAME); ITNAME.i++)
#define v2_arr_each(ARRPTR, ITNAME)   \
        v2_arr_it ITNAME = {0};       \
        v2_arr_next(ARRPTR, &ITNAME); \
        ITNAME.i++

static v2_arr *v2_arrcreate(int cap, void *(*allocfunc)(size_t))
{
        size_t  s   = sizeof(v2_arr) + sizeof(v2_i32) * cap;
        void   *mem = allocfunc(s);
        v2_arr *arr = (v2_arr *)mem;
        arr->data   = (v2_i32 *)(arr + 1);
        arr->n      = 0;
        arr->c      = cap;
        return arr;
}

static void v2_arrpush(v2_arr *arr, v2_i32 v)
{
        ASSERT(arr->n < arr->c);
        arr->data[arr->n++] = v;
}

static void v2_arrput(v2_arr *arr, v2_i32 v, int i)
{
        ASSERT(0 <= i && i <= arr->n);
        if (i == arr->n) {
                v2_arrpush(arr, v);
        } else {
                arr->data[i] = v;
        }
}

static void v2_arradd(v2_arr *arr, v2_i32 v)
{
        v2_arrpush(arr, v);
}

static void v2_arrinsert(v2_arr *arr, v2_i32 v, int i)
{
        ASSERT(arr->n < arr->c && i <= arr->n);
        for (int n = arr->n; n > i; n--) {
                arr->data[n] = arr->data[n - 1];
        }
        arr->data[i] = v;
        arr->n++;
}

static void v2_arrinsertq(v2_arr *arr, v2_i32 v, int i)
{
        ASSERT(arr->n < arr->c && i <= arr->n);
        arr->data[arr->n++] = arr->data[i];
        arr->data[i]        = v;
}

static int v2_arrfind(v2_arr *arr, v2_i32 v)
{
        for (int i = 0; i < arr->n; i++) {
                if (arr->data[i].x == v.x && arr->data[i].y == v.y) {
                        return i;
                }
        }
        return -1;
}

static bool32 v2_arrcontains(v2_arr *arr, v2_i32 v)
{
        return (v2_arrfind(arr, v) >= 0);
}

static void v2_arrpushunique(v2_arr *arr, v2_i32 v)
{
        if (!v2_arrcontains(arr, v)) {
                v2_arrpush(arr, v);
        }
}

static void v2_arrclr(v2_arr *arr)
{
        arr->n = 0;
}

static bool32 v2_arrdelatq(v2_arr *arr, int i)
{
        if (!(0 <= i && i < arr->n)) return 0;
        arr->data[i] = arr->data[--arr->n];
        return 1;
}

static bool32 v2_arrdelat(v2_arr *arr, int i)
{
        if (!(0 <= i && i < arr->n)) return 0;
        arr->n--;
        for (int n = i; n < arr->n; n++) {
                arr->data[n] = arr->data[n + 1];
        }
        return 1;
}

static v2_i32 v2_arrat(v2_arr *arr, int i)
{
        ASSERT(0 <= i && i < arr->n);
        return arr->data[i];
}

static v2_i32 *v2_arratp(v2_arr *arr, int i)
{
        ASSERT(0 <= i && i < arr->n);
        return &arr->data[i];
}

static v2_i32 v2_arrpop(v2_arr *arr)
{
        ASSERT(arr->n > 0);
        return arr->data[--arr->n];
}

static bool32 v2_arrdel(v2_arr *arr, v2_i32 v)
{
        int i = v2_arrfind(arr, v);
        if (i < 0) return 0;
        v2_arrdelat(arr, i);
        return 1;
}

static bool32 v2_arrdelq(v2_arr *arr, v2_i32 v)
{
        int i = v2_arrfind(arr, v);
        if (i < 0) return 0;
        v2_arrdelatq(arr, i);
        return 1;
}

static int v2_arrlen(v2_arr *arr)
{
        return arr->n;
}

#endif