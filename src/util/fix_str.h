// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef FIX_STR_H
#define FIX_STR_H

#include "pltf/pltf_types.h"

typedef struct {
    i32 l;
    i32 c;
    u8 *s;
} str8_s;

str8_s str8_alloc(i32 c, alloc_s a)
{
    str8_s r = {0};
    r.s      = (u8 *)a.allocf(a.ctx, sizeof(u8) * c);
    if (!r.s) return r;
    r.c = c;
    return r;
}

static bool32 str8_append(str8_s *dst, str8_s *src)
{
    if (!dst || !src) return 0;
    if (dst->c - dst->l < src->l) return 0;

    for (u32 n = 0; n < src->l; n++) {
        dst->s[dst->l++] = src->s[n];
    }
    return 1;
}

static bool32 c_str_from_str8(str8_s *s, char *buf, u32 bufsize)
{
    if (!s || !buf || bufsize == 0) return 0;
    if (bufsize <= s->l) return 0;

    for (u32 n = 0; n < s->l; n++) {
        buf[n] = s->s[n];
    }
    buf[s->l] = '\0';
    return 1;
}

static bool32 str8_append_c_str(str8_s *dst, const char *buf)
{
    if (!dst || !buf) return 0;
    u32 strl = 0;
    for (const char *c = buf; *c != '\0'; c++) {
        strl++;
    }

    if (dst->c - dst->l < strl) return 0;
    for (u32 n = 0; n < strl; n++) {
        dst->s[dst->l++] = buf[n];
    }
    return 1;
}

#define FIX_STR_DEF(L)                                                          \
    typedef struct {                                                            \
        i32 l;                                                                  \
        u8  s[L];                                                               \
    } str8_l##L##_s;                                                            \
                                                                                \
    static str8_s str8_from_str8_l##L(str8_l##L##_s *s)                         \
    {                                                                           \
        str8_s r = {0};                                                         \
        r.l      = s->l;                                                        \
        r.c      = L;                                                           \
        r.s      = &s->s[0];                                                    \
        return r;                                                               \
    }                                                                           \
                                                                                \
    static str8_s str8_from_str8_l##L##_cpy(str8_l##L##_s *s, u32 c, alloc_s a) \
    {                                                                           \
        str8_s r = str8_alloc(c, a);                                            \
        if (r.c) {                                                              \
            str8_s sc = str8_from_str8_l##L(s);                                 \
            str8_append(&r, &sc);                                               \
        }                                                                       \
        return r;                                                               \
    }

#endif