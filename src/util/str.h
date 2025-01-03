// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef STR_H
#define STR_H

#include "pltf/pltf.h"

static u32 hash_str(const char *str)
{
    u32 h = 0;
    for (const char *s = str; *s != '\0'; s++) {
        h = h * 31 + (u32)tolower(*s);
    }
    return h;
}

typedef struct {
    u32 l;
    u32 c;
    u8 *s;
} str8_s;

str8_s str8_alloc(u32 c, alloc_s a)
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

#define STR_DEF_LEN(L)                                                          \
    typedef struct {                                                            \
        u32 l;                                                                  \
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

#define FILEPATH_GEN(NAME, PATHNAME, FILENAME) \
    char NAME[128];                            \
    str_cpy(NAME, PATHNAME);                   \
    str_append(NAME, FILENAME)

static bool32 char_is_any(int c, const char *chars)
{
    if (!chars) return 0;
    for (const char *a = chars; *a != '\0'; a++) {
        if (c == (int)*a) return 1;
    }
    return 0;
}

static inline bool32 str_eq(const char *a, const char *b)
{
    return (strcmp(a, b) == 0);
}

// ignore lower/upper case when comparing
static bool32 str_eq_nc(const char *a, const char *b)
{
    for (const char *x = a, *y = b;; x++, y++) {
        if (((uint)(*x) & B8(11011111)) != ((uint)(*y) & B8(11011111)))
            return 0;
        if (*x == '\0') break;
    }
    return 1;
}

static inline bool32 str_contains(const char *str, const char *sequence)
{
    return (strstr(str, sequence) != NULL);
}

// number of characters excluding null-char
static inline i32 str_len(const char *a)
{
    return (i32)strlen(a);
}

static inline void str_cpy(char *dst, const char *src)
{
    strcpy(dst, src);
}

static void str_cpys(char *dst, u32 dstsize, const char *src)
{
    char       *d = dst;
    const char *s = src;
    while (*s != '\0' && (d + 1) < (dst + dstsize))
        *d++ = *s++;
    *d = '\0';
}

// assets/tex/file.png -> file.png
static void str_extract_filename(const char *src, char *buf, u32 bufsize)
{
    const char *s = src;
    while (*s != '\0' && *s != '.')
        s++;
    while (src < s) {
        s--;
        if (*s == '/') {
            s++;
            break;
        }
    }
    str_cpys(buf, bufsize, s);
}

// appends string b -> overwrites null-char and places a new null-char
static inline void str_append(char *dst, const char *src)
{
    strcat(dst, src);
}

// appends string b -> overwrites null-char and places a new null-char
static void str_append_i(char *dst, i32 i)
{
    if (i == 0) {
        str_append(dst, "0");
        return;
    }
    char b1[16] = {0};
    char b2[16] = {0};
    i32  l      = 0;
    for (i32 j = 0 <= i ? +i : -i; 0 < j; j /= 10) {
        b1[l++] = '0' + (j % 10);
    }

    i32 k1 = 0;
    if (i < 0) {
        k1    = 1;
        b2[0] = '-';
    }
    for (i32 k = 0; k < l; k++) {
        b2[k + k1] = b1[l - k - 1];
    }
    str_append(dst, &b2[0]);
}

static i32 num_from_hex(i32 c)
{
    switch (c) {
    case '0': return 0;
    case '1': return 1;
    case '2': return 2;
    case '3': return 3;
    case '4': return 4;
    case '5': return 5;
    case '6': return 6;
    case '7': return 7;
    case '8': return 8;
    case '9': return 9;
    case 'A':
    case 'a': return 10;
    case 'B':
    case 'b': return 11;
    case 'C':
    case 'c': return 12;
    case 'D':
    case 'd': return 13;
    case 'E':
    case 'e': return 14;
    case 'F':
    case 'f': return 15;
    }
    return -1;
}

static i32 hex_from_num(i32 c)
{
    static const char *g_hex = "0123456789ABCDEF";
    return (0x0 <= c && c <= 0xF ? g_hex[c] : 0);
}

static f32 f32_from_str(const char *str)
{
    const char *c = str;
    while (isspace((int)*c))
        c++;

    f32 res  = 0.f;
    f32 fact = 1.f;
    if (*c == '-') {
        fact = -1.f;
        c++;
    }

    for (i32 pt = 0;; c++) {
        if (*c == '.') {
            pt = 1;
        } else if (isdigit((int)*c)) {
            if (pt) fact *= .1f;
            res = res * 10.f + (f32)num_from_hex(*c);
        } else {
            break;
        }
    }
    return res * fact;
}

static i32 i32_from_str(const char *str)
{
    const char *c = str;
    while (isspace((int)*c))
        c++;
    i32 res = 0;
    i32 s   = +1;
    if (*c == '-') {
        s = -1;
        c++;
    }
    while ('0' <= *c && *c <= '9') {
        res *= 10;
        res += *c - '0';
        c++;
    }
    return (res * s);
}

static u32 u32_from_str(const char *str)
{
    const char *c = str;
    while (isspace((int)*c))
        c++;
    u32 res = 0;
    while ('0' <= *c && *c <= '9') {
        res *= 10;
        res += *c - '0';
        c++;
    }
    return res;
}

#define strs_from_u32(V, BUF) str_from_u32(V, BUF, sizeof(BUF))
static i32 str_from_u32(u32 v, char *buf, u32 bufsize)
{
    if (!buf || bufsize < 2) return 0;
    if (v == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return 2;
    }
    i32  n     = 0;
    char b[16] = {0};

    for (u32 x = v; x && (u32)n < bufsize; x /= 10) {
        b[n++] = '0' + (x % 10);
    }

    i32 len = --n;
    while (0 <= n) { // reverse
        buf[len - n] = b[n];
        n--;
    }
    buf[len + 1] = '\0';
    return len + 1;
}

// string float to fixed point integer parsing
static int QX_gen(const char *str, int q)
{
    int         neg = 0;
    const char *c   = str;
    if (*c == '-') {
        neg = 1;
        c++;
    }

    int n = 0;
    while (*c != '.' && *c != '\0') {
        n = n * 10 + ((*c++ - '0') << q);
    }

    if (*c == '.') {
        c++;
        uint f   = 0;
        uint div = 1;
        while (*c != '\0') {
            f = f * 10 + ((*c++ - '0') << q);
            div *= 10;
        }
        n += (int)(f / div);
    }

    return (neg ? -n : +n);
}

#define Q_4(NUM)  (int)((NUM) * 16.f)
#define Q_8(NUM)  (int)((NUM) * 256.f)
#define Q_16(NUM) (int)((NUM) * 65536.f)

#endif