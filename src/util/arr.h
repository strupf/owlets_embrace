// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "sys/sys_types.h"

#ifndef ARR_H
#define ARR_H

#define ARR_ASSERT assert

typedef int dummy;

typedef struct {
    dummy *data;
    int    n, c;
} dummy_arr;

dummy_arr *dummy_arrcreate(int cap, void *(*allocfunc)(usize size));
void       dummy_arrpush(dummy_arr *arr, dummy v);
void       dummy_arrinsert(dummy_arr *arr, dummy v, int i);
void       dummy_arrinsertq(dummy_arr *arr, dummy v, int i);
void       dummy_arrclr(dummy_arr *arr);
void       dummy_arrdelatq(dummy_arr *arr, int i);
void       dummy_arrdelat(dummy_arr *arr, int i);
dummy      dummy_arrat(dummy_arr *arr, int i);
dummy     *dummy_arratp(dummy_arr *arr, int i);
dummy      dummy_arrpop(dummy_arr *arr);
int        dummy_arrlen(dummy_arr *arr);

#define ARR_DEFINITION(NAME, T)                                           \
    typedef struct {                                                      \
        T  *data;                                                         \
        int n, c;                                                         \
    } arr_##NAME;                                                         \
                                                                          \
    arr_##NAME *arrcreate_dummy(int cap, void *(*allocfunc)(usize size)); \
    void        arrpush_dummy(arr_##NAME *arr, T v);                      \
    void        arrinsert_dummy(arr_##NAME *arr, T v, int i);             \
    void        arrinsertq_dummy(arr_##NAME *arr, T v, int i);            \
    void        arrclr_dummy(arr_##NAME *arr);                            \
    void        arrdelatq_dummy(arr_##NAME *arr, int i);                  \
    void        arrdelat_dummy(arr_##NAME *arr, int i);                   \
    T           arrat_dummy(arr_##NAME *arr, int i);                      \
    T          *arratp_dummy(arr_##NAME *arr, int i);                     \
    T           arrpop_dummy(arr_##NAME *arr);                            \
    int         arrlen_dummy(arr_##NAME *arr);

typedef struct {
    dummy *data;
    int    n, c;
} dummy_arr;

dummy_arr *arrcreate_dummy(int cap, void *(*allocfunc)(usize size));
void       arrpush_dummy(dummy_arr *arr, dummy v);
void       arrinsert_dummy(dummy_arr *arr, dummy v, int i);
void       arrinsertq_dummy(dummy_arr *arr, dummy v, int i);
void       arrclr_dummy(dummy_arr *arr);
void       arrdelatq_dummy(dummy_arr *arr, int i);
void       arrdelat_dummy(dummy_arr *arr, int i);
dummy      arrat_dummy(dummy_arr *arr, int i);
dummy     *arratp_dummy(dummy_arr *arr, int i);
dummy      arrpop_dummy(dummy_arr *arr);
int        arrlen_dummy(dummy_arr *arr);

dummy_arr *dummy_arrcreate(int cap, void *(*allocfunc)(usize size))
{
    usize      s   = sizeof(dummy_arr) + sizeof(dummy) * cap;
    void      *mem = allocfunc(s);
    dummy_arr *arr = (dummy_arr *)mem;
    arr->data      = (dummy *)(arr + 1);
    arr->n         = 0;
    arr->c         = cap;
    return arr;
}

void dummy_arrpush(dummy_arr *arr, dummy v)
{
    ARR_ASSERT(arr->n < arr->c);
    arr->data[arr->n++] = v;
}

void dummy_arrinsert(dummy_arr *arr, dummy v, int i)
{
    ARR_ASSERT(arr->n < arr->c && i <= arr->n);
    for (int n = arr->n; n > i; n--)
        arr->data[n] = arr->data[n - 1];
    arr->data[i] = v;
    arr->n++;
}

void dummy_arrinsertq(dummy_arr *arr, dummy v, int i)
{
    ARR_ASSERT(arr->n < arr->c && i <= arr->n);
    arr->data[arr->n++] = arr->data[i];
    arr->data[i]        = v;
}

void dummy_arrclr(dummy_arr *arr)
{
    arr->n = 0;
}

void dummy_arrdelatq(dummy_arr *arr, int i)
{
    ARR_ASSERT(0 <= i && i < arr->n);
    arr->data[i] = arr->data[--arr->n];
}

void dummy_arrdelat(dummy_arr *arr, int i)
{
    ARR_ASSERT(0 <= i && i < arr->n);
    arr->n--;
    for (int n = i; n < arr->n; n++)
        arr->data[n] = arr->data[n + 1];
}

dummy dummy_arrat(dummy_arr *arr, int i)
{
    ARR_ASSERT(0 <= i && i < arr->n);
    return arr->data[i];
}

dummy *dummy_arratp(dummy_arr *arr, int i)
{
    ARR_ASSERT(0 <= i && i < arr->n);
    return &arr->data[i];
}

dummy dummy_arrpop(dummy_arr *arr)
{
    ARR_ASSERT(arr->n > 0);
    return arr->data[--arr->n];
}

int dummy_arrlen(dummy_arr *arr)
{
    return arr->n;
}

#define ARR_DEF_BASE(NAME, T)                                             \
    typedef struct {                                                      \
        T  *data;                                                         \
        int n, c;                                                         \
    } NAME##_arr;                                                         \
                                                                          \
    static NAME##_arr *NAME##_arrcreate(int cap, void *(*allocf)(size_t)) \
    {                                                                     \
        usize       s   = sizeof(NAME##_arr) + sizeof(T) * cap;           \
        void       *mem = allocf(s);                                      \
        NAME##_arr *arr = (NAME##_arr *)mem;                              \
        arr->data       = (T *)(arr + 1);                                 \
        arr->n          = 0;                                              \
        arr->c          = cap;                                            \
        return arr;                                                       \
    }                                                                     \
                                                                          \
    static void NAME##_arrpush(NAME##_arr *arr, T v)                      \
    {                                                                     \
        assert(arr->n < arr->c);                                          \
        arr->data[arr->n++] = v;                                          \
    }                                                                     \
                                                                          \
    static void NAME##_arrinsert(NAME##_arr *arr, T v, int i)             \
    {                                                                     \
        assert(arr->n < arr->c && i <= arr->n);                           \
        for (int n = arr->n; n > i; n--)                                  \
            arr->data[n] = arr->data[n - 1];                              \
        arr->data[i] = v;                                                 \
        arr->n++;                                                         \
    }                                                                     \
                                                                          \
    static void NAME##_arrinsertq(NAME##_arr *arr, T v, int i)            \
    {                                                                     \
        assert(arr->n < arr->c && i <= arr->n);                           \
        arr->data[arr->n++] = arr->data[i];                               \
        arr->data[i]        = v;                                          \
    }                                                                     \
                                                                          \
    static void NAME##_arrclr(NAME##_arr *arr)                            \
    {                                                                     \
        arr->n = 0;                                                       \
    }                                                                     \
                                                                          \
    static bool32 NAME##_arrdelatq(NAME##_arr *arr, int i)                \
    {                                                                     \
        if (!(0 <= i && i < arr->n)) return 0;                            \
        arr->data[i] = arr->data[--arr->n];                               \
        return 1;                                                         \
    }                                                                     \
                                                                          \
    static bool32 NAME##_arrdelat(NAME##_arr *arr, int i)                 \
    {                                                                     \
        if (!(0 <= i && i < arr->n)) return 0;                            \
        arr->n--;                                                         \
        for (int n = i; n < arr->n; n++)                                  \
            arr->data[n] = arr->data[n + 1];                              \
        return 1;                                                         \
    }                                                                     \
                                                                          \
    static T NAME##_arrat(NAME##_arr *arr, int i)                         \
    {                                                                     \
        assert(0 <= i && i < arr->n);                                     \
        return arr->data[i];                                              \
    }                                                                     \
                                                                          \
    static T *NAME##_arratp(NAME##_arr *arr, int i)                       \
    {                                                                     \
        assert(0 <= i && i < arr->n);                                     \
        return &arr->data[i];                                             \
    }                                                                     \
                                                                          \
    static T NAME##_arrpop(NAME##_arr *arr)                               \
    {                                                                     \
        assert(arr->n > 0);                                               \
        return arr->data[--arr->n];                                       \
    }                                                                     \
                                                                          \
    static int NAME##_arrlen(NAME##_arr *arr)                             \
    {                                                                     \
        return arr->n;                                                    \
    }

#define _ARR_DEF_EXTENSION(NAME, T)                        \
    static bool32 NAME##_arrcontains(NAME##_arr *arr, T v) \
    {                                                      \
        return (NAME##_arrfind(arr, v) >= 0);              \
    }                                                      \
                                                           \
    static void NAME##_arrpushunique(NAME##_arr *arr, T v) \
    {                                                      \
        if (!NAME##_arrcontains(arr, v))                   \
            NAME##_arrpush(arr, v);                        \
    }                                                      \
                                                           \
    static bool32 NAME##_arrdel(NAME##_arr *arr, T v)      \
    {                                                      \
        int i = NAME##_arrfind(arr, v);                    \
        if (i < 0) return 0;                               \
        NAME##_arrdelat(arr, i);                           \
        return 1;                                          \
    }                                                      \
                                                           \
    static bool32 NAME##_arrdelq(NAME##_arr *arr, T v)     \
    {                                                      \
        int i = NAME##_arrfind(arr, v);                    \
        if (i < 0) return 0;                               \
        NAME##_arrdelatq(arr, i);                          \
        return 1;                                          \
    }

#define ARR_DEF_PRIMITIVE(NAME, T)                  \
    ARR_DEF_BASE(NAME, T)                           \
    static int NAME##_arrfind(NAME##_arr *arr, T v) \
    {                                               \
        for (int i = 0; i < arr->n; i++)            \
            if (arr->data[i] == v)                  \
                return i;                           \
        return -1;                                  \
    }                                               \
    _ARR_DEF_EXTENSION(NAME, T)

// CMP = compare if two elements are equal
// e.g.: T: int, int intcmp(int *a, int *b); -> return true if equal
#define ARR_DEF(NAME, T, CMP)                       \
    ARR_DEF_BASE(NAME, T)                           \
    static int NAME##_arrfind(NAME##_arr *arr, T v) \
    {                                               \
        for (int i = 0; i < arr->n; i++)            \
            if (CMP(&arr->data[i], &v))             \
                return i;                           \
        return -1;                                  \
    }                                               \
    _ARR_DEF_EXTENSION(NAME, T)

// =============================================================================

#endif