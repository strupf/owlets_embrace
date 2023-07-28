/* =============================================================================
* Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
* This source code is licensed under the GPLv3 license found in the
* LICENSE file in the root directory of this source tree.
============================================================================= */
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
#endif